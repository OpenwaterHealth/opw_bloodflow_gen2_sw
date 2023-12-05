#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <openssl/evp.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/aes.h>
#include <openssl/rand.h>

#include "encrypt_data.h"

#define ERR_EVP_CIPHER_INIT -1
#define ERR_EVP_CIPHER_UPDATE -2
#define ERR_EVP_CIPHER_FINAL -3
#define ERR_EVP_CTX_NEW -4

#define AES_256_KEY_SIZE 32
#define AES_BLOCK_SIZE 16
#define BUFSIZE 1024

#define RSA_SIZE 512

unsigned char key[AES_256_KEY_SIZE];

unsigned char plaintext[RSA_SIZE];
unsigned char ciphertext[RSA_SIZE];
size_t plaintext_size;
RSA *pRSA;

#define CRYPTO_PRINT_ERROR fprintf(stderr, "[x] %s\n", strerror(errno))

const char *public_key =
    "-----BEGIN PUBLIC KEY-----\n\
MIICIjANBgkqhkiG9w0BAQEFAAOCAg8AMIICCgKCAgEAzVcfXrdVuWKQE6ZFvhXS\n\
Aqs98Gvpqlr6bUUsnPOD87iL55waIduPdonz+X4Y2g/asn6ekMhY2ZJZlz1xy1Yy\n\
9Tpr1Nh5jve+N+7/mVoqjz/qBK0v7XkDsY3sQPD+tBrQugyaGB98JWR1mk8Q5CKE\n\
p8iRTg8W0DQPmuv5xeYAvQf0ZjWyb1gx4YMdOYg6jAa7lyjiqow/DaQCPuHhHsju\n\
2X8qTFpRI9XKf9dsIYZAjTcL9Ez72HijBQTH+HChhoTmizcCRGhqi/AMAYtermY5\n\
v1/cDyGjZJIDZ00H28NZmveYf9CbDa/DuaY4kVEWNT+I8dsW0JfpmicHyPlAl2Tj\n\
kUPZQ3/UCran0bzr/Um+0mmX2LelKYwWM8fysnQ5BvHhVz4kOn5jjvoViyv9OGMg\n\
CtAlkPw8u40+T9/wkYHUyBN8jKyvVOj9ZeQUUhHcog2E5h8ULFVq+ucgfVAbp7r4\n\
XjHyb9ufiMgePZ49HBTQi6ZVAjJ+enH++bMwOY38GJ4/sVGEs57HYRXyeH49Po2k\n\
6rrdE+ar6Y1JD1zTFCQXyEkEN0jtIJEVyJMp5mU3ccnyfoqpkJVthoCyZo7KwoyD\n\
JY6k6XxxIiit7pVQk4bxKl74HrEurxoXXiGlRwzmDTuVlFvbJsDVFSDjIxyo7c4e\n\
eQnnKCNoDmAWZqzyMbKh0o0CAwEAAQ==\n\
-----END PUBLIC KEY-----\n";

void binToHex (const unsigned char *buff, size_t length, char *output, size_t out_length)
{
    char binHex[] = "0123456789ABCDEF";

    assert(buff);
    assert(length > 0);
    assert(output);
    assert(out_length > (2 * length));

    *output = '\0';
    for (; length > 0; --length, out_length -= 2)
    {
        unsigned char byte = *buff++;

        *output++ = binHex[(byte >> 4) & 0x0F];
        *output++ = binHex[byte & 0x0F];
    }
    *output++ = '\0';
}

int readPublicKeyBuffer(void *src, size_t src_size)
{
    FILE *fp = fmemopen(src, src_size, "rb");
    if (fp == NULL)
    {
        CRYPTO_PRINT_ERROR;
        return 0;
    }
    if (PEM_read_RSA_PUBKEY(fp, &pRSA, NULL, NULL) == NULL)
    {
        CRYPTO_PRINT_ERROR;
        return 0;
    }
    fclose(fp);
    return 1;
}

int setPlainText(void *src, size_t src_size)
{
    if (src_size > RSA_SIZE)
    {
        fprintf(stderr,
                "[x] the data size of %d bytes exceeds the limit of %d bytes\n",
                (int)src_size,
                (int)RSA_size(pRSA));
        return 0;
    }
    plaintext_size = src_size;
    memcpy(plaintext, src, plaintext_size);
    return 1;
}

int publicEncrypt(void *src, size_t src_size)
{
    if (setPlainText(src, src_size) == 0)
    {
        return 0;
    }
    int result = RSA_public_encrypt(plaintext_size,
                                    (unsigned char *)plaintext,
                                    (unsigned char *)ciphertext,
                                    pRSA,
                                    RSA_PKCS1_PADDING);
    if (result < 0)
    {
        CRYPTO_PRINT_ERROR;
        return 0;
    }
    return 1;
}

int encryptData(char *ofile_name, void *data, size_t data_size)
{
    char ofile_name_encrypted[256];
    char ofile_name_key_encrypted[256];
    FILE *f_encrypted;

    sprintf(ofile_name_encrypted, "%s.encrypted", ofile_name);
    sprintf(ofile_name_key_encrypted, "%s_key.encrypted", ofile_name);

    /* Generate cryptographically strong pseudo-random bytes for key and IV */
    if (!RAND_bytes(key, sizeof(key)))
    {
        /* OpenSSL reports a failure, act accordingly */
        fprintf(stderr, "ERROR: RAND_bytes error: %s\n", strerror(errno));
        return errno;
    }

    /* Allow enough space in output buffer for additional block */
    int cipher_block_size = EVP_CIPHER_block_size(EVP_aes_256_cbc());
    unsigned char out_buf[BUFSIZE + cipher_block_size];

    int data_pos = 0;
    int num_bytes_to_read;
    int out_len;
    EVP_CIPHER_CTX *ctx;

    ctx = EVP_CIPHER_CTX_new();
    if (ctx == NULL)
    {
        fprintf(stderr, "ERROR: EVP_CIPHER_CTX_new failed. OpenSSL error: %s\n", ERR_error_string(ERR_get_error(), NULL));
        return ERR_EVP_CTX_NEW;
    }

    /* Don't set key or IV right away; we want to check lengths */
    if (!EVP_CipherInit_ex(ctx, EVP_aes_256_cbc(), NULL, NULL, NULL, 1))
    {
        fprintf(stderr, "ERROR: EVP_CipherInit_ex failed. OpenSSL error: %s\n", ERR_error_string(ERR_get_error(), NULL));
        return ERR_EVP_CTX_NEW;
    }

    assert(EVP_CIPHER_CTX_key_length(ctx) == AES_256_KEY_SIZE);
    assert(EVP_CIPHER_CTX_iv_length(ctx) == AES_BLOCK_SIZE);

    /* Now we can set key, no need for IV since key is generated and only ever used once */
    if (!EVP_CipherInit_ex(ctx, NULL, NULL, key, NULL, 1))
    {
        fprintf(stderr, "ERROR: EVP_CipherInit_ex failed. OpenSSL error: %s\n", ERR_error_string(ERR_get_error(), NULL));
        // EVP_CIPHER_CTX_cleanup(ctx);
        return ERR_EVP_CIPHER_INIT;
    }

    /* Open and truncate file to zero length or create ciphertext file for writing */
    f_encrypted = fopen(ofile_name_encrypted, "wb");
    if (!f_encrypted)
    {
        /* Unable to open file for writing */
        fprintf(stderr, "ERROR: fopen error: %s\n", strerror(errno));
        return errno;
    }

    while (data_pos < (int)data_size)
    {
        num_bytes_to_read = (data_pos + BUFSIZE > (int)data_size ? (int)data_size - data_pos : BUFSIZE);

        if (!EVP_CipherUpdate(ctx, out_buf, &out_len, data + data_pos, num_bytes_to_read))
        {
            fprintf(stderr, "ERROR: EVP_CipherUpdate failed. OpenSSL error: %s\n", ERR_error_string(ERR_get_error(), NULL));
            // EVP_CIPHER_CTX_cleanup(ctx);
            fclose(f_encrypted);
            return ERR_EVP_CIPHER_UPDATE;
        }
        fwrite(out_buf, sizeof(unsigned char), out_len, f_encrypted);
        if (ferror(f_encrypted))
        {
            fprintf(stderr, "ERROR: fwrite error: %s\n", strerror(errno));
            // EVP_CIPHER_CTX_cleanup(ctx);
            fclose(f_encrypted);
            return errno;
        }
        data_pos += num_bytes_to_read;
    }

    /* Now cipher the final block and write it out to file */
    if (!EVP_CipherFinal_ex(ctx, out_buf, &out_len))
    {
        fprintf(stderr, "ERROR: EVP_CipherFinal_ex failed. OpenSSL error: %s\n", ERR_error_string(ERR_get_error(), NULL));
        // EVP_CIPHER_CTX_cleanup(ctx);
        fclose(f_encrypted);
        return ERR_EVP_CIPHER_FINAL;
    }
    fwrite(out_buf, sizeof(unsigned char), out_len, f_encrypted);
    if (ferror(f_encrypted))
    {
        fprintf(stderr, "ERROR: fwrite error: %s\n", strerror(errno));
        // EVP_CIPHER_CTX_cleanup(ctx);
        fclose(f_encrypted);
        return errno;
    }
    fclose(f_encrypted);

    char keyBuff[2 * RSA_SIZE + 1];
    binToHex(key, sizeof(key), keyBuff, sizeof(keyBuff));

    pRSA = RSA_new();
    readPublicKeyBuffer((void *)public_key, strlen(public_key));
    assert(RSA_size(pRSA) == RSA_SIZE);
    publicEncrypt(keyBuff, strlen(keyBuff));
    RSA_free(pRSA);

    f_encrypted = fopen(ofile_name_key_encrypted, "wb");
    if (!f_encrypted)
    {
        /* Unable to open file for writing */
        fprintf(stderr, "ERROR: fopen error: %s\n", strerror(errno));
        return errno;
    }
    fwrite(ciphertext, sizeof(char), sizeof(ciphertext), f_encrypted);

    return 0;
}
