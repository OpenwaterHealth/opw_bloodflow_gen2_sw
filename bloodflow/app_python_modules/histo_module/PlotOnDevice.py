#%%
import glob, struct, os, re, argparse, io
import numpy as np
import matplotlib
import matplotlib.pyplot as plt
import base64

class ChannelData(object):
    '''
    Class to hold data for a single channel
    '''
    def __init__(self):
        self.imagePathLaserOff = None
        self.imageLaserOffHistWidth = None
        self.imagePathLaserOn  = None
        self.histogramPath     = None
        self.dataAvailable     = True
        self.correctedMean     = None
        self.contrast  = None
        self.imageMean = None
        self.finalCamTemp = 0
        
class ReadGen2Data:
    '''
    Class to read in data from Gen2 camera
    '''
    def __init__(self, pathIn, posIn=1, deviceID=-1, correctionTypeIn=0, scanTypeIn=4, enablePlotsIn=True):
        self.path = pathIn
        self.scanType = scanTypeIn
        self.pos = int(posIn)-1
        #0 - Initial device scan order Right Horizontal, Left Horizontal, Right Temple, Left Temple
        #1 - Simultaneous scan order Horizontal, Near, Temple, Horizontal Repeat
        #2 - Long scan which is n times repeat of the Horizontal
        #3 - Four channel scan order Horizontal+Vertical, Horizontal+Near, Vertial+Near, Horizontal+Vertical Repeat
        #All processing is done Ch0 and then Ch1
        self.cameraGain = [[16,16,16,16,1,1,1,1],[16,1,16,16,16,1,16,16],[16,16],
                           [16,16,1,16,
                            1,16,16,1,
                            16,16,1,16,
                            1,16,16,1],
                            [16,1,16,16,1,16]] 
        self.cameraPosition = [['RH','LH','RV','LV','RN','LN','RN','LN',],
                               ['RH','LN','RV','RH','LH','RN','LV','LH',],
                                ['RH','LH'],
                                ['RH','RH','LN','RH',
                                 'LN','RV','RV','LN',
                                 'LH','LH','RN','LH',
                                 'RN','LV','LV','RN',],
                                 ['RH','LN','RV','LH','RN','LV']
                               ]
        self.correctionType = correctionTypeIn
        self.printStats = False
        self.enablePlots = enablePlotsIn
        self.imageWidth = 1920
        self.histLength  = 1032 #Includes the last four numbers added on
        self.numBinsHist = 1024
        self.darkBinThresh = [256,128]
        self.hiGainSetting = []
        self.noisyBinMin = 100
        self.ADCgain = 0.12 # photons/electrons
        self.dt = 0.025
        self.minbpm = 30
        self.maxbpm = 180
        self.deviceID = deviceID

        #Order is P1Ch0, P2Ch0, P3Ch0, P4Ch0
        self.channelData = [ ChannelData() for i in range(6) ]

    def ReadHistogram(self, histogramPath):
        '''
        Read in histogram data from a file
        '''
        histogramFile = open(histogramPath, 'rb')
        histogramData1 = histogramFile.read()
        histogramFile.close()
        
        histogramnp = np.frombuffer(histogramData1, dtype=np.uint32)
        histogramData = np.copy(histogramnp)
        histogramData = histogramData.reshape((histogramnp.shape[0]//(self.numBinsHist+8),self.numBinsHist+8))
        histogramData = histogramData[:,:-8]
        histogramData[:,14:18] = 0
        
        histogramnp = np.frombuffer(histogramData1, dtype=np.float32)
        histogramData1 = np.copy(histogramnp)
        histogramData1 = histogramData1.reshape((histogramnp.shape[0]//(self.numBinsHist+8),self.numBinsHist+8))
        obMean = np.copy(histogramData1[:,16])

        histogramu8 = np.frombuffer(histogramData1, dtype=np.uint8)
        histogramTemp = np.copy(histogramu8)
        histogramTemp = histogramTemp.reshape((int(histogramu8.shape[0]/4)//(self.histLength),self.histLength*4))
        camTemps = np.zeros(histogramTemp.shape)       
        camTemps = np.flip(histogramTemp[:,4104:4112],axis=1)
        camInd = int(re.split('scan_ch_', histogramPath)[1][0])
        if(self.scanType <3): 
            camTemps[:,[1,3]] = camTemps[:,[1,3]] * 1.5625 - 45 # full camera temperature data output
            camTemps = camTemps[:,camInd*2+1] # selecting single camera's temp
        else:
            camTemps[:,camInd] = camTemps[:,camInd] * 1.5625 - 45 # full camera temperature data output
            camTemps = camTemps[:,camInd] # selecting single camera's temp
        
        finalCamTemp = camTemps[-109] # grab the final camera temperature
        return histogramData, obMean, finalCamTemp
    
    def GetHistogramStats(self, hist, bins):
        hist[hist<self.noisyBinMin] = 0
        mean = np.dot(hist,bins)/np.sum(hist)
        binsSq = np.multiply(bins,bins)
        var = (np.dot(hist,binsSq)-mean*mean*np.sum(hist))/(np.sum(hist)-1)
        std = np.sqrt(var)
        
        histWid = np.sum(hist>100)
        
        return mean, std, histWid

    def GetImageStats(self, image):
        histDark, bins = np.histogram(image[:20,:], bins=list(range(self.numBinsHist)))
        histDark = histDark[:-1]; bins = bins[:-2] #Remove the 1023 bin
        darkMean, darkStd, lsrOffObWidth = self.GetHistogramStats(histDark, bins)

        histExp, bins = np.histogram(image[20:,:], bins=list(range(self.numBinsHist)))
        histExp = histExp[:-1]; bins = bins[:-2]
        lsrOffMean, lsrOffStd, lsrOffWidth = self.GetHistogramStats(histExp, bins)

        return lsrOffMean, lsrOffStd, lsrOffWidth, darkMean, darkStd, lsrOffObWidth

    def ReadImage(self, imagePath):
        imageFile = open(imagePath, 'rb')
        imageData = imageFile.read()
        imageFile.close()
        if len(imageData)%self.imageWidth:
            print('Incomplete image file. Skipping',imagePath)
            imageData = np.array([])
        else:
            imageData = np.array(struct.unpack('<'+str(int(len(imageData)/2))+'H',imageData)).reshape((int(len(imageData)/(self.imageWidth*2)),self.imageWidth))
        return imageData

    def natural_sort(self,l):
        convert = lambda text: int(text) if text.isdigit() else text.lower()
        alphanum_key = lambda key: [convert(c) for c in re.split('([0-9]+)', key)]
        return sorted(l, key=alphanum_key)

    def CheckFolderForHistogramsAndImages(self):
        chHistFiles = []
        chImageFiles = []
        for i in range(6):
            histogramPatternCh = os.path.join( self.path, "histo_output_fullscan_ch_{}*.bin".format(i) )
            histFiles = self.natural_sort(glob.glob(histogramPatternCh))
            if len(histFiles) == 1:
                chHistFiles.append(histFiles[self.pos])
            else:
                errorString = 'Missing histogram files for channel '+str(i)+' in the folder:'+self.path 
                errorString += ' Files found: ' + str(len(histFiles))
                print(errorString)
                raise ValueError(errorString)
                
            imagePatternCh = os.path.join( self.path, "csix_raw_output_ch_{}*_exp0_1920x*_10bit_bayer.y".format(i) )
            imageFiles = self.natural_sort(glob.glob(imagePatternCh))
            if len(imageFiles)==2:
                imageFiles = imageFiles
            else:
                errorString = 'Missing image files for channel '+str(i)+' in the folder:'+self.path
                print(errorString)
                print(len(imageFiles))
                raise ValueError(errorString)
            chImageFiles.append(imageFiles[2*self.pos])
            chImageFiles.append(imageFiles[2*self.pos+1])

        return chHistFiles, chImageFiles

    def ReadHistogramAndImageFileNames(self):
        chHistPaths, chImages= self.CheckFolderForHistogramsAndImages()

        if len(chHistPaths)!=6:
            print('Missing histogram files in the folder:'+self.path)
            errorString = 'Missing histogram files in the folder:'+self.path
            raise ValueError(errorString)
            
        for ind, histFileName in enumerate(chHistPaths):
            self.channelData[ind].histogramPath = histFileName

        for ind in range(0, len(chImages), 2):
            self.channelData[int(ind/2)].imagePathLaserOff = chImages[ind]
            self.channelData[int(ind/2)].imagePathLaserOn  = chImages[ind+1]

        return

    def ComputeContrastForChannel(self, channelData, gain, cameraPositionStr):
        scanHistograms, obMeanScan, finalCamTemp = self.ReadHistogram(channelData.histogramPath)
        if len(scanHistograms)==0:
            channelData.dataAvailable = False
            return
        imgLaserOff = self.ReadImage(channelData.imagePathLaserOff)
        lsrOffMean, lsrOffStd, lsrOffWidth, lsrOffObMean, lsrOffObStd, lsrOffObWidth = self.GetImageStats(imgLaserOff)
        channelData.imageLaserOffHistWidth = lsrOffWidth

        imgLaserOn  = self.ReadImage(channelData.imagePathLaserOn)
        lsrOnMean, lsrOnStd, lsrOnWidth, lsrOnObMean, lsrOnObStd, lsrOnObWidth = self.GetImageStats(imgLaserOn)
        
        bins = np.array(list(range(scanHistograms.shape[1]-1)))
        histStatsTups = [ self.GetHistogramStats(scanHistograms[i,:-1],bins) for i in range(scanHistograms.shape[0]) ]
        histStatsTups = list(zip(*histStatsTups))
        scanMean = np.array(histStatsTups[0])
        scanStd  = np.array(histStatsTups[1])

        if lsrOffObMean<150 and gain!=1:
            gain = 1
            print('Gain reset to 1 for channel ',cameraPositionStr)
        elif lsrOffObMean>150 and gain!=16:
            gain = 16
            print('Gain reset to 16 for channel ',cameraPositionStr)

        #mean correction: scan mean - scan ob row pixels fit to line + offset between main row pixels and ob row pixels in img when laser is off
        #variance correction: scan variance - variance main when laser off - gain*mean
        t = range(0, len(obMeanScan))
        p = np.polyfit(t, obMeanScan, 1)
        obMeanScanFit = np.polyval(p, t)
        channelData.correctedMean = scanMean-obMeanScanFit-(lsrOffMean-lsrOffObMean)
        if np.any(channelData.correctedMean<0):
            channelData.correctedMean = scanMean-obMeanScanFit
            print('Negative correctedMean in channel. Turning off dark frame offset correction for channel ',cameraPositionStr)
        varCorrected = scanStd**2-lsrOffStd**2-self.ADCgain*gain*channelData.correctedMean
        if np.any(varCorrected<0):
            varCorrected = scanStd**2
            print('Negative variance in channel with correction. Turning off variance correction for channel ',cameraPositionStr)
        contrast = np.sqrt(varCorrected)/(channelData.correctedMean)

        channelData.contrast = contrast
        channelData.channelPosition = cameraPositionStr
        channelData.lsrOffObWidth = lsrOffObWidth
        channelData.finalCamTemp = finalCamTemp

    def ReadDataAndComputeContrast(self):
        self.ReadHistogramAndImageFileNames()
        
        for ind,channel in enumerate(self.channelData):
            self.ComputeContrastForChannel(channel, self.cameraGain[self.scanType][ind],
                                            self.cameraPosition[self.scanType][ind])


def generate_data(filepath, scanPos, display=False):   
    dt = 1./40
    scanData = ReadGen2Data(filepath, posIn=scanPos, scanTypeIn=3)
    scanData.ReadDataAndComputeContrast()

    pltInds = [0,3,4,1,2,5]
    data = []
    for i in range(0, 6):
        chNum = i
        channel = scanData.channelData[chNum]
        if not channel.dataAvailable:
            print("Error Channel {} NOT AVAILABLE".format(chNum))
            continue

        tmp = np.array([channel.correctedMean, channel.contrast], dtype=np.float32)
        tmpBytes = tmp.tobytes()
        data.append(tmpBytes)
        
    # Join the byte arrays into a single byte buffer
    dataBytes = b''.join(data)

    # Return the buffer
    return dataBytes

def generate_plot(filepath, scanPos, display=False,showStatistics=False):  
    dt = 1./40
    scanData = ReadGen2Data(filepath, posIn=scanPos, scanTypeIn=4)
    scanData.ReadDataAndComputeContrast()
    

    plt.figure().clear()
    # plt.subplots_adjust(wspace=.3,hspace=.3,left=.07, right=.93,top=.95,bottom=.05)
    plt.subplots_adjust(hspace=.3, top=.95,bottom=.05)
    pltInds = [0,3,4,1,2,5]
    for i in range(0,6):
        chNum = i
        channel = scanData.channelData[chNum]
        mean_mean = np.average(channel.correctedMean)
        mean_std = np.std(channel.correctedMean)
        contrast_mean = np.average(channel.contrast)
        contrast_std = np.std(channel.contrast)

        if not channel.dataAvailable:
            print("Error Channel {} NOT AVAILABLE".format(chNum))
            continue
        ticks = np.linspace(0,len(channel.contrast)*dt,len(channel.contrast))        

        ax1 = plt.subplot(6, 1, i+1)
        # ax1.set_ylabel(f"%s (%s° C)" % (channel.channelPosition,channel.finalCamTemp))
        ax1.set_xlim(0,15)
        if(i!=5): ax1.get_xaxis().set_visible(False)
        ax2 = ax1.twinx()
        ax2.plot(ticks,channel.correctedMean,'r')
        ax2.tick_params(axis='y',color='red', labelsize=11)
        ax2.yaxis.label.set_color('red')
        ax2.invert_yaxis()
        ax1.plot(ticks,channel.contrast,'k')
        ax1.invert_yaxis()
        ax1.tick_params(axis='y', labelsize=11)
        
        ax1.yaxis.set_major_locator(matplotlib.ticker.MaxNLocator(nbins=2,min_n_ticks=2)) 
        ax2.yaxis.set_major_locator(matplotlib.ticker.MaxNLocator(nbins=2,min_n_ticks=2)) 
        plt.title(f"%s (%s° C)" % (channel.channelPosition,channel.finalCamTemp))

        if(showStatistics):
            textstr = '\n'.join((
                r'$\mu_{\mu}=%.2f$' % (mean_mean, ),
                r'$\mu_{\sigma}=%.2f$' % (mean_std, ),
                r'$C_{\mu}=%.4f$' % (contrast_mean, ),
                r'$C_{\sigma}=%.4f$' % (contrast_std, )))

            props = dict(boxstyle='round', facecolor='wheat', alpha=1.0)
            ax2.text(0.05, 0.95, textstr, transform=ax2.transAxes, fontsize=14,
                    verticalalignment='top', bbox=props)


        plt.title(channel.channelPosition)
        plt.ylabel(f"%s° C" % channel.finalCamTemp, color='r', fontsize=11)

    plt.suptitle(' ')
    fig = plt.gcf()
    #Set figure to 1200x800 pixels
    fig.set_dpi(50)
    fig.set_size_inches(12, 8)

    if display:
        name = 'scanPlot'+str(scanPos)+'.png'
        plt.savefig(name)
        #plt.show()
        return None
    else:
        # Save the plot to a buffer
        print("Saving image to buffer")
        buffer = io.BytesIO()
        plt.savefig(buffer, format='jpg')
        buffer.seek(0)
        print("Base64 encoiding image data")
        b64_data = base64.b64encode(buffer.getvalue()).decode('ascii')
        print("Foramtting message buffer")
        msg = '{{"P{}C4":"{}"}}'.format(scanPos, b64_data)
        print("MESSAGE LEN: {}".format(len(msg)))
        bytes_io = io.BytesIO(msg.encode())
        print("returning buffer")
        # Return the buffer
        return bytes_io.getvalue()

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Compute contrast for a given scan')
    parser.add_argument('--scanPath', type=str, help='Path to the scan folder')
    parser.add_argument('--scanType', type=str, help='Type of scan: "fullscan" or "linearity"')
    parser.add_argument('--dt', type=str, help='Laser time period in seconds')
    parser.add_argument('--scanPos', type=int, help='Position to process')
    args = parser.parse_args()
    if args.scanPath is None:
        args.scanPath = '/home/ethanhead/Projects/Gen2Analysis/2023_10_25_171736_6ch/FULLSCAN_6C_2023_10_25_171836'
    if args.scanType is None:
        args.scanType = 4
    if args.scanPos is None:
        args.scanPos = 1
    if args.dt is None:
        args.dt = 1./40

    generate_plot(args.scanPath, args.scanPos, True)
    