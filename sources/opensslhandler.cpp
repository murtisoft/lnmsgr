/****************************************************************************
**
** This file is part of LAN Messenger.
**
** Copyright (c) LAN Messenger Authors.
** 
** LAN Messenger is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** LAN Messenger is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with LAN Messenger.  If not, see <http://www.gnu.org/licenses/>.
**
****************************************************************************/

#include "zdebuglog.h"
#include "opensslhandler.h"

lmCrypto::lmCrypto(void) {
    pKey = NULL;
    encryptMap.clear();
    decryptMap.clear();
    bits = 1024;       //This has to be 1024, otherwise it will crash older clients <=1.2.39
    exponent = 65537;
}

lmCrypto::~lmCrypto(void) {
    EVP_PKEY_free(pKey);
}

// helper to safely replace a context in a map (hopefully finally solving context orphaning issues.)
void replaceCtx(QMap<QString, EVP_CIPHER_CTX*>& map, const QString& userId, EVP_CIPHER_CTX* newCtx) {
    if (map.contains(userId)) {
        EVP_CIPHER_CTX_free(map.value(userId));
    }
    map.insert(userId, newCtx);
}

//	creates an RSA key pair and returns the string representation of the public key
QByteArray lmCrypto::generateRSA(void) {
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new_from_name(NULL, "RSA", NULL);
    EVP_PKEY_keygen_init(ctx);
    EVP_PKEY_CTX_set_rsa_keygen_bits(ctx, bits);

    BIGNUM* bn = BN_new();
    BN_set_word(bn, exponent);
    EVP_PKEY_CTX_set1_rsa_keygen_pubexp(ctx, bn);
    BN_free(bn);

    EVP_PKEY_generate(ctx, &pKey);
    EVP_PKEY_CTX_free(ctx);

    BIO* bio = BIO_new(BIO_s_mem());
    PEM_write_bio_RSAPublicKey(bio, EVP_PKEY_get0_RSA(pKey));           // for compatibility with older clients
    int keylen = BIO_pending(bio);
    char* pem_key = (char*)calloc(keylen + 1, 1);
    BIO_read(bio, pem_key, keylen);
    publicKey = QByteArray(pem_key, keylen);
    BIO_free_all(bio);
    free(pem_key);

    return publicKey;
}

//	generates a random aes key and iv, and encrypts it with the public key
QByteArray lmCrypto::generateAES(QString* lpszUserId, QByteArray& pubKey) {
    char* pemKey = pubKey.data();
    BIO* bio = BIO_new_mem_buf(pemKey, pubKey.length());
    RSA* r = PEM_read_bio_RSAPublicKey(bio, NULL, NULL, NULL);          // for compatibility with older clients
    BIO_free_all(bio);
    EVP_PKEY* rsa = EVP_PKEY_new();
    EVP_PKEY_assign_RSA(rsa, r);                                        // for compatibility with older clients

    int keyDataLen = 32;
    unsigned char* keyData = (unsigned char*)malloc(keyDataLen);
    RAND_bytes(keyData, keyDataLen);

    int keyLen = 32;
    int ivLen = EVP_CIPHER_iv_length(EVP_aes_256_cbc());
    int keyIvLen = keyLen + ivLen;
    unsigned char* keyIv = (unsigned char*)malloc(keyIvLen);

    int rounds = 5;
    keyLen = EVP_BytesToKey(EVP_aes_256_cbc(), EVP_sha1(), NULL, keyData, keyDataLen, rounds, keyIv, keyIv + keyLen);

    EVP_CIPHER_CTX* ectx = EVP_CIPHER_CTX_new();
    EVP_CIPHER_CTX* dctx = EVP_CIPHER_CTX_new();
    EVP_EncryptInit_ex(ectx, EVP_aes_256_cbc(), NULL, keyIv, keyIv + keyLen);
    replaceCtx(encryptMap, *lpszUserId, ectx);
    EVP_DecryptInit_ex(dctx, EVP_aes_256_cbc(), NULL, keyIv, keyIv + keyLen);
    replaceCtx(decryptMap, *lpszUserId, dctx);

    EVP_PKEY_CTX* rsaCtx = EVP_PKEY_CTX_new(rsa, NULL);
    EVP_PKEY_encrypt_init(rsaCtx);
    EVP_PKEY_CTX_set_rsa_padding(rsaCtx, RSA_PKCS1_OAEP_PADDING);

    size_t eKeyIvLen = 0;
    EVP_PKEY_encrypt(rsaCtx, NULL, &eKeyIvLen, keyIv, keyIvLen);
    unsigned char* eKeyIv = (unsigned char*)malloc(eKeyIvLen);
    EVP_PKEY_encrypt(rsaCtx, eKeyIv, &eKeyIvLen, keyIv, keyIvLen);

    QByteArray baKeyIv((char*)eKeyIv, eKeyIvLen);

    EVP_PKEY_CTX_free(rsaCtx);
    EVP_PKEY_free(rsa);  // also frees r
    free(keyIv);
    free(eKeyIv);
    free(keyData);

    return baKeyIv;
}

//	decrypts the aes key and iv with the private key
void lmCrypto::retreiveAES(QString* lpszUserId, QByteArray& aesKeyIv) {
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(pKey, NULL);
    EVP_PKEY_decrypt_init(ctx);
    EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING);

    size_t keyIvLen = 0;
    EVP_PKEY_decrypt(ctx, NULL, &keyIvLen, (unsigned char*)aesKeyIv.data(), aesKeyIv.length());
    unsigned char* keyIv = (unsigned char*)malloc(keyIvLen);
    EVP_PKEY_decrypt(ctx, keyIv, &keyIvLen, (unsigned char*)aesKeyIv.data(), aesKeyIv.length());
    EVP_PKEY_CTX_free(ctx);

    int keyLen = 32;
    EVP_CIPHER_CTX* ectx = EVP_CIPHER_CTX_new();
    EVP_CIPHER_CTX* dctx = EVP_CIPHER_CTX_new();
    EVP_EncryptInit_ex(ectx, EVP_aes_256_cbc(), NULL, keyIv, keyIv + keyLen);
    replaceCtx(encryptMap, *lpszUserId, ectx);
    EVP_DecryptInit_ex(dctx, EVP_aes_256_cbc(), NULL, keyIv, keyIv + keyLen);
    replaceCtx(decryptMap, *lpszUserId, dctx);
    free(keyIv);
}

QByteArray lmCrypto::encrypt(QString* lpszUserId, QByteArray& clearData) {
    int outLen = clearData.length() + AES_BLOCK_SIZE;
    unsigned char* outBuffer = (unsigned char*)malloc(outLen);
    if (outBuffer == NULL) {
        lmDebugLog::write("Error: Buffer not allocated");
        return QByteArray();
    }
    int foutLen = 0;

    EVP_CIPHER_CTX* ctx = encryptMap.value(*lpszUserId);
    if (EVP_EncryptInit_ex(ctx, NULL, NULL, NULL, NULL)) {
        if (EVP_EncryptUpdate(ctx, outBuffer, &outLen, (unsigned char*)clearData.data(), clearData.length())) {
            if (EVP_EncryptFinal_ex(ctx, outBuffer + outLen, &foutLen)) {
                outLen += foutLen;
                QByteArray byteArray((char*)outBuffer, outLen);
                free(outBuffer);
                return byteArray;
            }
        }
    }
    lmDebugLog::write("Error: Message encryption failed");
    free(outBuffer);
    return QByteArray();
}

QByteArray lmCrypto::decrypt(QString* lpszUserId, QByteArray& cipherData) {
    int outLen = cipherData.length();
    unsigned char* outBuffer = (unsigned char*)malloc(outLen);
    if (outBuffer == NULL) {
        lmDebugLog::write("Error: Buffer not allocated");
        return QByteArray();
    }
    int foutLen = 0;

    EVP_CIPHER_CTX* ctx = decryptMap.value(*lpszUserId);
    if (EVP_DecryptInit_ex(ctx, NULL, NULL, NULL, NULL)) {
        if (EVP_DecryptUpdate(ctx, outBuffer, &outLen, (unsigned char*)cipherData.data(), cipherData.length())) {
            if (EVP_DecryptFinal_ex(ctx, outBuffer + outLen, &foutLen)) {
                outLen += foutLen;
                QByteArray byteArray((char*)outBuffer, outLen);
                free(outBuffer);
                return byteArray;
            }
        }
    }
    lmDebugLog::write("Error: Message decryption failed");
    free(outBuffer);
    return QByteArray();
}
