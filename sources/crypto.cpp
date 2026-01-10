/****************************************************************************
**
** This file is part of LAN Messenger.
**
** Copyright (c) LAN Messenger Contributors.
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

#include "crypto.h"
#include "trace.h"

lmCrypto::lmCrypto(void) {
    pKey = nullptr;
    encryptMap.clear();
    decryptMap.clear();
    bits = 1024;       //This has to be 1024, otherwise it will crash older clients <=1.2.39
    exponent = 65537;
}

lmCrypto::~lmCrypto(void) {
    EVP_PKEY_free(pKey);

    // Free all allocated encryption contexts
    foreach (EVP_CIPHER_CTX* ctx, encryptMap) {
        EVP_CIPHER_CTX_free(ctx);
    }
    encryptMap.clear();

    // Free all allocated decryption contexts
    foreach (EVP_CIPHER_CTX* ctx, decryptMap) {
        EVP_CIPHER_CTX_free(ctx);
    }
    decryptMap.clear();
}

//	creates an RSA key pair and returns the string representation of the public key
QByteArray lmCrypto::generateRSA() {
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, nullptr);
    if (!ctx) return QByteArray();

    EVP_PKEY_keygen_init(ctx);
    EVP_PKEY_CTX_set_rsa_keygen_bits(ctx, bits);
    EVP_PKEY_keygen(ctx, &pKey);
    EVP_PKEY_CTX_free(ctx);

    BIO* bio = BIO_new(BIO_s_mem());
    if (PEM_write_bio_RSAPublicKey(bio, EVP_PKEY_get0_RSA(pKey))) {     // for compatibility with older clients
        publicKey.resize(BIO_pending(bio));
        BIO_read(bio, publicKey.data(), publicKey.size());
    }

    BIO_free(bio);
    return publicKey;
}

//	generates a random aes key and iv, and encrypts it with the public key
QByteArray lmCrypto::generateAES(QString* lpszUserId, QByteArray& pubKey) {
    BIO* bio = BIO_new_mem_buf(pubKey.data(), pubKey.length());
    EVP_PKEY* peerPubKey = PEM_read_bio_PUBKEY(bio, nullptr, nullptr, nullptr);
    BIO_free(bio);

    unsigned char keyIv[48]; // 32 (key) + 16 (iv)
    RAND_bytes(keyIv, 48);

    EVP_CIPHER_CTX *ectx = EVP_CIPHER_CTX_new(), *dctx = EVP_CIPHER_CTX_new();
    EVP_EncryptInit_ex(ectx, EVP_aes_256_cbc(), nullptr, keyIv, keyIv + 32);
    EVP_DecryptInit_ex(dctx, EVP_aes_256_cbc(), nullptr, keyIv, keyIv + 32);

    encryptMap.insert(*lpszUserId, ectx);
    decryptMap.insert(*lpszUserId, dctx);

    EVP_PKEY_CTX* encCtx = EVP_PKEY_CTX_new(peerPubKey, nullptr);
    EVP_PKEY_encrypt_init(encCtx);
    EVP_PKEY_CTX_set_rsa_padding(encCtx, RSA_PKCS1_OAEP_PADDING);

    size_t outlen;
    EVP_PKEY_encrypt(encCtx, nullptr, &outlen, keyIv, 48);
    QByteArray encryptedKeyIv(outlen, 0);
    EVP_PKEY_encrypt(encCtx, (unsigned char*)encryptedKeyIv.data(), &outlen, keyIv, 48);

    EVP_PKEY_CTX_free(encCtx);
    EVP_PKEY_free(peerPubKey);
    return encryptedKeyIv;
}

//	decrypts the aes key and iv with the private key
void lmCrypto::retreiveAES(QString* lpszUserId, QByteArray& aesKeyIv) {
    EVP_PKEY_CTX* decCtx = EVP_PKEY_CTX_new(pKey, nullptr);
    EVP_PKEY_decrypt_init(decCtx);
    EVP_PKEY_CTX_set_rsa_padding(decCtx, RSA_PKCS1_OAEP_PADDING);

    size_t outlen;
    EVP_PKEY_decrypt(decCtx, nullptr, &outlen, (unsigned char*)aesKeyIv.data(), aesKeyIv.length());

    unsigned char keyIv[48];
    if (EVP_PKEY_decrypt(decCtx, keyIv, &outlen, (unsigned char*)aesKeyIv.data(), aesKeyIv.length()) <= 0) {
        lmTrace::write("Error: RSA Decryption of AES key failed");
        EVP_PKEY_CTX_free(decCtx);
        return;
    }
    EVP_PKEY_CTX_free(decCtx);

    EVP_CIPHER_CTX *ectx = EVP_CIPHER_CTX_new();
    EVP_CIPHER_CTX *dctx = EVP_CIPHER_CTX_new();

    EVP_EncryptInit_ex(ectx, EVP_aes_256_cbc(), nullptr, keyIv, keyIv + 32);
    EVP_DecryptInit_ex(dctx, EVP_aes_256_cbc(), nullptr, keyIv, keyIv + 32);

    encryptMap.insert(*lpszUserId, ectx);
    decryptMap.insert(*lpszUserId, dctx);
}

QByteArray lmCrypto::encrypt(QString* lpszUserId, QByteArray& clearData) {
    EVP_CIPHER_CTX* ctx = encryptMap.value(*lpszUserId);
    if (!ctx) return QByteArray();

    QByteArray out(clearData.length() + AES_BLOCK_SIZE, 0);
    if (out.isEmpty()) {
        lmTrace::write("Error: Buffer not allocated");
        return QByteArray();
    }

    int len = 0, flen = 0;
    EVP_EncryptInit_ex(ctx, nullptr, nullptr, nullptr, nullptr);

    if (!EVP_EncryptUpdate(ctx, (unsigned char*)out.data(), &len, (const unsigned char*)clearData.data(), clearData.length()) ||
        !EVP_EncryptFinal_ex(ctx, (unsigned char*)out.data() + len, &flen)) {
        lmTrace::write("Error: Message encryption failed");
        return QByteArray();
    }

    out.resize(len + flen);
    return out;
}

QByteArray lmCrypto::decrypt(QString* lpszUserId, QByteArray& cipherData) {
    EVP_CIPHER_CTX* ctx = decryptMap.value(*lpszUserId);
    if (!ctx) return QByteArray();

    QByteArray out(cipherData.length(), 0);
    if (out.isEmpty() && cipherData.length() > 0) {
        lmTrace::write("Error: Buffer not allocated");
        return QByteArray();
    }

    int len = 0, flen = 0;
    EVP_DecryptInit_ex(ctx, nullptr, nullptr, nullptr, nullptr);

    if (!EVP_DecryptUpdate(ctx, (unsigned char*)out.data(), &len, (const unsigned char*)cipherData.data(), cipherData.length()) ||
        !EVP_DecryptFinal_ex(ctx, (unsigned char*)out.data() + len, &flen)) {
        lmTrace::write("Error: Message decryption failed");
        return QByteArray();
    }

    out.resize(len + flen);
    return out;
}
