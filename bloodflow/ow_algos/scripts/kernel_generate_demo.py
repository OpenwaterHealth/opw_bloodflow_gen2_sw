from tiovx import *

code = KernelExportCode("ow_algos", Core.C66, "VISION_APPS_PATH")

kernel = Kernel("ow_calc_histo")

# image
kernel.setParameter(Type.IMAGE, Direction.INPUT, ParamState.REQUIRED, "input")
kernel.setParameter(Type.DISTRIBUTION, Direction.OUTPUT, ParamState.REQUIRED, "histo", ['VX_TYPE_UINT32'])

kernel.setTarget(Target.DSP1)
kernel.setTarget(Target.DSP2)

code.export(kernel)