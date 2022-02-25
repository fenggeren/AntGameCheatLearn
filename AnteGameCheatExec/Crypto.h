#pragma  once

#include "botan_all.h"
#include <vector>
#include <string>

/*
������
	char* inBuff��ԭ��
	int inLen ��ԭ�ĳ���
*/
void Md5Hash(const char* inBuff, int inLen, Botan::SecureVector<unsigned char> &out);

/*
������
       std::string passphrase ������
	   const char* inBuff��ԭ��
	   int inLen ��ԭ�ĳ���
	   Botan::SecureVector<unsigned char> &out�����ܺ������
*/
void Aes128Crypto(std::string passphrase, const char* inBuff, int inLen, Botan::SecureVector<unsigned char> &out);



/*
������
	std::string passphrase ������
	const char* inBuff������
	int inLen �����ĳ���
	Botan::SecureVector<unsigned char> &out�����ܺ������
*/
void Aes128UnCrypto(std::string passphrase, const char* inBuff, int inLen, Botan::SecureVector<unsigned char> &out);

/*
����:
	char* buff,���ĳ���
	int nLen, ���ĳ���
����ֵ:
	base64�������
*/
std::string Base64Encode(const char* buff, int nLen);


/*
����:
	const char* inputBuff, ����
	int nInputLen ���ĳ���
	Botan::SecureVector<unsigned char> &out ԭ��
*/
void Base64Decode(const char* inputBuff, int nInputLen, Botan::SecureVector<unsigned char> &out);

/*
������
	Botan::SecureVector<unsigned char> &publicKey, ���ɹ�Կ
	Botan::SecureVector<unsigned char> &privateKey�� ����˽Կ
*/
void GenRsaKey(Botan::SecureVector<unsigned char> &publicKey, Botan::SecureVector<unsigned char> &privateKey);


/*
����:
	Botan::SecureVector<unsigned char> &out:�ڿ���̨չʾ���ֽ�
*/
void ShowSecureVector(Botan::SecureVector<unsigned char> &out);


/*
	���ܣ�չʾʹ��
*/
void Useage();
