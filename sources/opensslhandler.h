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

#ifndef OPENSSLHANDLER_H
#define OPENSSLHANDLER_H

#include <QMap>
#include <QString>
#include <QByteArray>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rand.h>
#include <openssl/aes.h>
#include <openssl/hmac.h>

class lmOpenSSL
{
public:
    lmOpenSSL();
    ~lmOpenSSL();

    QByteArray generateRSA();
    QByteArray generateAES(QString* lpszUserId, QByteArray& pubKey);
    void retreiveAES(QString* lpszUserId, QByteArray& aesKeyIv);
    QByteArray encrypt(QString* lpszUserId, QByteArray& clearData);
    QByteArray decrypt(QString* lpszUserId, QByteArray& cipherData);

    //  Per-packet encryption for datagrams (voice). Uses the same session key as text,
    //  but is stateless so lost or reordered packets do not break decryption.
    QByteArray deriveVoiceKey(const QString& userId, const QByteArray& salt);
    QByteArray sealPacket(const QByteArray& key, quint32 seq, const QByteArray& aad, const QByteArray& clearData);
    QByteArray openPacket(const QByteArray& key, quint32 seq, const QByteArray& aad, const QByteArray& cipherData);

    QByteArray publicKey;

private:
    EVP_PKEY* pKey;

    QMap<QString, EVP_CIPHER_CTX*> encryptMap;
    QMap<QString, EVP_CIPHER_CTX*> decryptMap;
    QMap<QString, QByteArray> sessionKeys;

    int bits;
    long exponent;
};

#endif // OPENSSLHANDLER_H
