#include "core/CryptoHelper.h"

#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/sha.h>

namespace CryptoHelper {

bool derivarChaveEIV(const QByteArray &salt, const QString &senha,
                     QByteArray &chave, QByteArray &iv)
{
    QByteArray derived(DERIVED_LENGTH, 0);
    int res = PKCS5_PBKDF2_HMAC(
        senha.toUtf8().constData(), senha.toUtf8().length(),
        reinterpret_cast<const unsigned char*>(salt.constData()), salt.length(),
        PBKDF2_ITERATIONS, EVP_sha256(), DERIVED_LENGTH,
        reinterpret_cast<unsigned char*>(derived.data()));
    if (res != 1) return false;
    chave = derived.left(AES_KEY_LENGTH);
    iv    = derived.mid(AES_KEY_LENGTH, AES_IV_LENGTH);
    return true;
}

QByteArray encrypt(const QByteArray &plaintext,
                   const QByteArray &key,
                   const QByteArray &iv)
{
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return {};
    QByteArray ciphertext(plaintext.size() + 32, 0);
    int len = 0, totalLen = 0;
    EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr,
                       reinterpret_cast<const unsigned char*>(key.constData()),
                       reinterpret_cast<const unsigned char*>(iv.constData()));
    EVP_EncryptUpdate(ctx, reinterpret_cast<unsigned char*>(ciphertext.data()), &len,
                      reinterpret_cast<const unsigned char*>(plaintext.constData()), plaintext.size());
    totalLen += len;
    EVP_EncryptFinal_ex(ctx, reinterpret_cast<unsigned char*>(ciphertext.data()) + len, &len);
    totalLen += len;
    EVP_CIPHER_CTX_free(ctx);
    ciphertext.resize(totalLen);
    return ciphertext;
}

QByteArray decrypt(const QByteArray &ciphertext,
                   const QByteArray &key,
                   const QByteArray &iv)
{
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return {};
    QByteArray plaintext(ciphertext.size() + 32, 0);
    int len = 0, totalLen = 0;
    EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr,
                       reinterpret_cast<const unsigned char*>(key.constData()),
                       reinterpret_cast<const unsigned char*>(iv.constData()));
    EVP_DecryptUpdate(ctx, reinterpret_cast<unsigned char*>(plaintext.data()), &len,
                      reinterpret_cast<const unsigned char*>(ciphertext.constData()), ciphertext.size());
    totalLen += len;
    if (EVP_DecryptFinal_ex(ctx, reinterpret_cast<unsigned char*>(plaintext.data()) + len, &len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return {};
    }
    totalLen += len;
    EVP_CIPHER_CTX_free(ctx);
    plaintext.resize(totalLen);
    return plaintext;
}

QByteArray gerarSalt()
{
    QByteArray salt(SALT_LENGTH, 0);
    RAND_bytes(reinterpret_cast<unsigned char*>(salt.data()), SALT_LENGTH);
    return salt;
}

} // namespace CryptoHelper
