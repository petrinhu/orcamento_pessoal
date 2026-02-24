#pragma once

#include <QByteArray>
#include <QString>

namespace CryptoHelper {

constexpr int AES_KEY_LENGTH    = 32;
constexpr int AES_IV_LENGTH     = 16;
constexpr int SALT_LENGTH       = 16;
constexpr int DERIVED_LENGTH    = AES_KEY_LENGTH + AES_IV_LENGTH;
constexpr int PBKDF2_ITERATIONS = 600000;

bool derivarChaveEIV(const QByteArray &salt, const QString &senha,
                     QByteArray &chave, QByteArray &iv);

QByteArray encrypt(const QByteArray &plaintext,
                   const QByteArray &key,
                   const QByteArray &iv);

QByteArray decrypt(const QByteArray &ciphertext,
                   const QByteArray &key,
                   const QByteArray &iv);

} // namespace CryptoHelper
