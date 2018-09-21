/*
   Copyright (C) 2015-2017 Alexandr Akulich <akulichalexander@gmail.com>

   This file is a part of TelegramQt library.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

 */

#include <QObject>

#include "CTelegramStream_p.hpp"
#include "CTelegramStreamExtraOperators.hpp"

#include "IODevicePtr.hpp"
#include "MTProto/Stream.hpp"

#include <QBuffer>
#include <QTest>
#include <QDebug>

#include <QtEndian>

template <typename T>
int getValueEncodedSize(const T &value)
{
    return sizeof(value);
}

template <>
int getValueEncodedSize(const QByteArray &value)
{
    const int payloadSize = (value.size() < 0xfe) ? value.size() + 1 : value.size() + 4;
    int padding = payloadSize & 3;
    if (padding) {
        return payloadSize + (4 - padding);
    }
    return payloadSize;
}

template <>
int getValueEncodedSize(const QString &value)
{
    return getValueEncodedSize(value.toUtf8());
}

template <typename T>
void writeDataHelper(void *&output, const void *&input, int &bytesCount)
{
    T *&outPtr = reinterpret_cast<T*&>(output);
    const T *&inPtr = reinterpret_cast<const T*&>(input);
    constexpr int sizeOfChunk = sizeof(T);
    while (bytesCount >= sizeOfChunk) {
        *outPtr = *inPtr;
        ++outPtr;
        ++inPtr;
        bytesCount -= sizeOfChunk;
    }
}

void writeData(void *output, const void *input, int count)
{
    writeDataHelper<quint64>(output, input, count);
    writeDataHelper<quint32>(output, input, count);
    writeDataHelper<quint16>(output, input, count);
    writeDataHelper<quint8>(output, input, count);
}

static const char s_nulls[4] = { 0, 0, 0, 0 };

template <typename T>
void packData(void *output, const T &value)
{
    *reinterpret_cast<T*>(output) = value;
}

void packShortData(char *output, const QByteArray &value)
{
    *output = value.size();
    ++output;
    writeData(output, value.data(), value.size());
    output += value.size();
    int extraBytes = (value.size() + 1) & 3;
    if (extraBytes) {
        writeData(output, s_nulls, 4 - extraBytes);
    }
}

void packLongData(char *output, const QByteArray &value)
{
    const quint32 sizeToWrite = quint32((value.size() << 8) + 0xfe);
    packData(output, sizeToWrite);
    output += 4;
    writeData(output, value.data(), value.size());
    output += value.size();
    int extraBytes = value.size() & 3;
    if (extraBytes) {
        writeData(output, s_nulls, 4 - extraBytes);
    }
}

template <>
void packData(void *output, const QByteArray &value)
{
    char *outPtr = reinterpret_cast<char*>(output);
    if (value.size() < 0xfe) {
        packShortData(outPtr, value);
    } else {
        packLongData(outPtr, value);
    }
}

template <>
void packData(void *output, const QString &value)
{
    packData(output, value.toUtf8());
}

template <typename T1>
QByteArray encodeData(const T1 &a1)
{
    const int size = getValueEncodedSize(a1);
    QByteArray result(size, Qt::Uninitialized);
    packData(result.data(), a1);
    return result;
}

template <typename T1, typename T2>
QByteArray encodeData(const T1 &a1, const T2 &a2)
{
    const int a2Offset = getValueEncodedSize(a1);
    const int size = a2Offset + getValueEncodedSize(a2);
    QByteArray result(size, Qt::Uninitialized);
    packData(result.data(), a1);
    packData(result.data() + a2Offset, a2);
    return result;
}

template <typename T1, typename T2, typename T3>
QByteArray encodeData(const T1 &a1, const T2 &a2, const T3 &a3)
{
    const int a2Offset = getValueEncodedSize(a1);
    const int a3Offset = a2Offset + getValueEncodedSize(a2);
    const int size = a3Offset + getValueEncodedSize(a3);
    QByteArray result(size, Qt::Uninitialized);
    packData(result.data(), a1);
    packData(result.data() + a2Offset, a2);
    packData(result.data() + a3Offset, a3);
    return result;
}

template <typename T1, typename T2, typename T3, typename T4>
QByteArray encodeData(const T1 &a1, const T2 &a2, const T3 &a3, const T4 &a4)
{
    const int a2Offset = getValueEncodedSize(a1);
    const int a3Offset = a2Offset + getValueEncodedSize(a2);
    const int a4Offset = a3Offset + getValueEncodedSize(a3);
    const int size = a4Offset + getValueEncodedSize(a4);
    QByteArray result(size, Qt::Uninitialized);
    packData(result.data(), a1);
    packData(result.data() + a2Offset, a2);
    packData(result.data() + a3Offset, a3);
    packData(result.data() + a4Offset, a4);
    return result;
}

template <typename T1, typename T2, typename T3, typename T4, typename T5>
QByteArray encodeData(const T1 &a1, const T2 &a2, const T3 &a3, const T4 &a4, const T5 &a5)
{
    const int a2Offset = getValueEncodedSize(a1);
    const int a3Offset = a2Offset + getValueEncodedSize(a2);
    const int a4Offset = a3Offset + getValueEncodedSize(a3);
    const int a5Offset = a4Offset + getValueEncodedSize(a4);
    const int size = a5Offset + getValueEncodedSize(a5);
    QByteArray result(size, Qt::Uninitialized);
    packData(result.data(), a1);
    packData(result.data() + a2Offset, a2);
    packData(result.data() + a3Offset, a3);
    packData(result.data() + a4Offset, a4);
    packData(result.data() + a5Offset, a5);
    return result;
}

struct STestData {
    QVariant value;
    QByteArray serializedData;
    STestData(QVariant v, QByteArray e) : value(v), serializedData(e) { }
};

template <typename T>
QByteArray tlObjectToBytes(const T &data)
{
    CTelegramStream innerStream(CTelegramStream::WriteOnly);
    innerStream << data;
    return innerStream.getData();
}

class tst_CTelegramStream : public QObject
{
    Q_OBJECT
public:
    explicit tst_CTelegramStream(QObject *parent = nullptr);

private slots:
    void testEncode();
    void benchmarkEncodeTLValuePlacement();
    void benchmarkEncodeTLValueStream();
    void benchmarkEncodeStream();
    void benchmarkEncodePlacement();
    void benchmarkEncodeStream1();
    void benchmarkEncodeStream2();
    void benchmarkEncodeStream3();
    void benchmarkEncodeStream4();
    void benchmarkEncodePlacement1();
    void benchmarkEncodePlacement2();
    void benchmarkEncodePlacement3();
    void benchmarkEncodePlacement4();
    void benchmarkReadWeakBytes_data();
    void benchmarkReadWeakBytes();
    void benchmarkReadBytes_data();
    void benchmarkReadBytes();
    void testReadRawBytes();
    void testReadRawBytesWithContent();
    void stringsLimitSerialization();
    void shortStringSerialization();
    void longStringSerialization();
    void intSerialization();
    void vectorOfIntsSerialization();
    void vectorDeserializationError();
    void pointerVectorSerialization();
    void pointerVectorDeserialization();
    void tlNumbersSerialization();
    void tlDcOptionDeserialization();
    void readError();
    void byteArrays();
    void reqPqData();
    void testStreamView_viewLocksAndSkip();
    void testStreamView_viewTakeBytes();
    void testStreamView_readOutOfView();
    void testStreamView_benchmarkView_data();
    void testStreamView_benchmarkView();
    void testStreamView_benchmarkCopy_data();
    void testStreamView_benchmarkCopy();

};

tst_CTelegramStream::tst_CTelegramStream(QObject *parent) :
    QObject(parent)
{
}

void tst_CTelegramStream::benchmarkReadWeakBytes_data()
{
    QTest::addColumn<int>("totalSize");
    QTest::addColumn<int>("partSize");

    QTest::newRow("small") << 32 << 8;
    QTest::newRow("medium") << 128 << 16;
    QTest::newRow("large") << 1024 << 256;
    QTest::newRow("extra_large") << 4096 << 1024;
}

void tst_CTelegramStream::benchmarkReadWeakBytes()
{
    QFETCH(int, totalSize);
    QFETCH(int, partSize);

    QByteArray data(totalSize, Qt::Uninitialized);
    CTelegramStream stream(data);

    QBENCHMARK {
        while (!stream.atEnd()) {
            QByteArray w1 = stream.readWeakBytes(partSize);
            QVERIFY(!w1.isEmpty());
        }
    }
}

void tst_CTelegramStream::benchmarkReadBytes_data()
{
    return benchmarkReadWeakBytes_data();
}

void tst_CTelegramStream::benchmarkReadBytes()
{
    QFETCH(int, totalSize);
    QFETCH(int, partSize);

    QByteArray data(totalSize, Qt::Uninitialized);
    CTelegramStream stream(data);

    QBENCHMARK {
        while (!stream.atEnd()) {
            QByteArray w1 = stream.readBytes(partSize);
            QVERIFY(!w1.isEmpty());
        }
    }
}

void tst_CTelegramStream::testEncode()
{
    const QStringList dataList = {
        QStringLiteral("1"), QStringLiteral("02"), QStringLiteral("003"), QStringLiteral("0004"),
        QStringLiteral("00005"), QStringLiteral("000006"), QStringLiteral("0000007"), QStringLiteral("00000008")
    };

    for (const QString &s : dataList) {
        CTelegramStream stream(CTelegramStream::WriteOnly);
        stream << s;
        const int writtenBytes = stream.getData().size();
        if (writtenBytes != getValueEncodedSize(s)) {
            qWarning() << "A problem with" << s;
        }
        QCOMPARE(writtenBytes, getValueEncodedSize(s));
    }
    for (const QString &s : dataList) {
        CTelegramStream stream(CTelegramStream::WriteOnly);
        stream << s;
        const QByteArray encodedStr = encodeData(s);
        const QByteArray streamedStr = stream.getData();

        if (streamedStr != encodedStr) {
            qWarning() << "A problem with" << s;
        }
        QCOMPARE(streamedStr.toHex(), encodedStr.toHex());
    }
    {
        const TLValue testValue = TLValue::AccountChangePhone;
        const QByteArray encoded = encodeData(testValue);
        CTelegramStream stream(encoded);
        TLValue readValue;
        stream >> readValue;
        QCOMPARE(testValue, readValue);
    }
    {
        const TLValue testValue = TLValue::AccountChangePhone;
        const QString phoneNumber = "myPhone"; // size = 7
        const QString phoneCodeHash = "7531"; // size = 4
        const QByteArray encoded = encodeData(testValue, phoneNumber, phoneCodeHash);

        CTelegramStream stream(encoded);
        TLValue readValue;
        QString readPhoneNumber;
        QString readPhoneCodeHash;
//        QString readPhoneCode;
        stream >> readValue;
        stream >> readPhoneNumber;
        stream >> readPhoneCodeHash;
        QCOMPARE(testValue, readValue);
        QCOMPARE(readPhoneNumber, phoneNumber);
        QCOMPARE(readPhoneCodeHash, phoneCodeHash);
    }
}

void tst_CTelegramStream::benchmarkEncodeTLValuePlacement()
{
    QBENCHMARK {
        const TLValue testValue = TLValue::AccountChangePhone;
        const QByteArray encoded = encodeData(testValue);
        Q_UNUSED(encoded)
    }
}
void tst_CTelegramStream::benchmarkEncodeTLValueStream()
{
    QBENCHMARK {
        CTelegramStream stream(CTelegramStream::WriteOnly);
        stream << TLValue::AccountChangePhone;
        const QByteArray encoded = stream.getData();
        Q_UNUSED(encoded)
    }
}

void tst_CTelegramStream::benchmarkEncodeStream()
{
    const QStringList dataList = {
        QStringLiteral("1"), QStringLiteral("02"), QStringLiteral("003"), QStringLiteral("0004"),
        QStringLiteral("00005"), QStringLiteral("000006"), QStringLiteral("0000007"), QStringLiteral("00000008")
    };

    QBENCHMARK {
        for (const QString &s : dataList) {
            CTelegramStream stream(CTelegramStream::WriteOnly);
            stream << s;
            stream.getData();
        }
    }
}

void tst_CTelegramStream::benchmarkEncodePlacement()
{
    const QStringList dataList = {
        QStringLiteral("1"), QStringLiteral("02"), QStringLiteral("003"), QStringLiteral("0004"),
        QStringLiteral("00005"), QStringLiteral("000006"), QStringLiteral("0000007"), QStringLiteral("00000008")
    };

    QBENCHMARK {
        for (const QString &s : dataList) {
            encodeData(s);
        }
    }
}

static const QString encArg1 = QStringLiteral("myphonenumber");
static const QString encArg2 = QStringLiteral("myphonecode");
static const quint64 encArg3 = 12345678ull;
static const TLValue encArg4 = TLValue::RpcResult;

void tst_CTelegramStream::benchmarkEncodeStream1()
{
    QBENCHMARK {
        CTelegramStream stream(CTelegramStream::WriteOnly);
        stream << encArg1;
        stream.getData();
    }
}

void tst_CTelegramStream::benchmarkEncodeStream2()
{
    QBENCHMARK {
        CTelegramStream stream(CTelegramStream::WriteOnly);
        stream << encArg1;
        stream << encArg2;
        stream << encArg2;

        stream << encArg2;
        stream << encArg2;
        stream.getData();
    }

}

void tst_CTelegramStream::benchmarkEncodeStream3()
{
    QBENCHMARK {
        CTelegramStream stream(CTelegramStream::WriteOnly);
        stream << encArg1;
        stream << encArg2;
        stream << encArg3;
        stream.getData();
    }
}

void tst_CTelegramStream::benchmarkEncodeStream4()
{
    QBENCHMARK {
        CTelegramStream stream(CTelegramStream::WriteOnly);
        stream << encArg1;
        stream << encArg2;
        stream << encArg3;
        stream << encArg4;
        stream.getData();
    }
}

void tst_CTelegramStream::benchmarkEncodePlacement1()
{
    QBENCHMARK {
        encodeData(encArg1);
    }
}

void tst_CTelegramStream::benchmarkEncodePlacement2()
{
    QBENCHMARK {
        encodeData(encArg1, encArg2,
                   encArg2,
                   encArg2,
                   encArg2
                   );
    }
}

void tst_CTelegramStream::benchmarkEncodePlacement3()
{
    QBENCHMARK {
        encodeData(encArg1, encArg2, encArg3);
    }
}

void tst_CTelegramStream::benchmarkEncodePlacement4()
{
    QBENCHMARK {
        encodeData(encArg1, encArg2, encArg3, encArg4);
    }
}

void tst_CTelegramStream::testReadRawBytes()
{
    const QByteArray c_data = QByteArrayLiteral("0123456789abcdef").toHex();
    constexpr int partSize = 4;

    CTelegramStream stream(c_data);
    QByteArray firstPart = stream.readBytes(partSize);
    QCOMPARE(firstPart, c_data.mid(partSize * 0, partSize));
    QByteArray secondPart = stream.readWeakBytes(partSize);
    QCOMPARE(secondPart, c_data.mid(partSize * 1, partSize));
    QByteArray thirdPart = stream.readBytes(partSize);
    QCOMPARE(thirdPart, c_data.mid(partSize * 2, partSize));
    QByteArray lastPart = stream.readWeakBytes();
    QCOMPARE(lastPart, c_data.mid(partSize * 3));
}

void tst_CTelegramStream::testReadRawBytesWithContent()
{
    // Prepare
    constexpr quint32 c_itemsCount = 3;
    constexpr quint64 c_ids[c_itemsCount] = { 1, 2, 3};
    constexpr quint32 c_ackNo[c_itemsCount] = { 10, 11, 12};
    const QByteArray c_innerData[3] = {
        tlObjectToBytes(TLFileLocation()),
        tlObjectToBytes(TLBadMsgNotification()),
        tlObjectToBytes(TLPong()),
    };

    CTelegramStream sourceDataStream(CTelegramStream::WriteOnly);
    {
        sourceDataStream << TLValue::MsgContainer;
        sourceDataStream << c_itemsCount;
        for (quint32 i = 0; i < c_itemsCount; ++i) {
            sourceDataStream << c_ids[i];
            sourceDataStream << c_ackNo[i];
            sourceDataStream << static_cast<quint32>(c_innerData[i].size());
            sourceDataStream.writeBytes(c_innerData[i]);
        }
    }
    const QByteArray bigData = sourceDataStream.getData();

    // Test
    CTelegramStream stream(bigData);
    TLValue type;
    stream >> type;
    QCOMPARE(type, TLValue::MsgContainer);
    quint32 itemsCount;
    stream >> itemsCount;
    QCOMPARE(itemsCount, c_itemsCount);

    for (quint32 i = 0; i < itemsCount; ++i) {
        quint64 messageId;
        quint32 sequenceNumber;
        quint32 contentLength;

        stream >> messageId;
        stream >> sequenceNumber;
        stream >> contentLength;

        QByteArray innerData = stream.readWeakBytes(contentLength);
        QCOMPARE(innerData.toHex(), c_innerData[i].toHex());
    }
    QVERIFY(stream.bytesAvailable() == 0);
}

void tst_CTelegramStream::shortStringSerialization()
{
    QList<STestData> data;

    const char serializedChars1[8] = { char(4), 't', 'e', 's', 't', 0, 0, 0 };
    const char serializedChars2[8] = { char(5), 't', 'e', 's', 't', '5', 0, 0 };
    const char serializedChars3[8] = { char(6), 't', 'e', 's', 't', '6', '6', 0 };
    const char serializedChars4[8] = { char(7), '7', 's', 'e', 'v', 'e', 'n', '7' };

    data.append(STestData(QLatin1String("test"), QByteArray(serializedChars1, sizeof(serializedChars1))));
    data.append(STestData(QLatin1String("test5"), QByteArray(serializedChars2, sizeof(serializedChars2))));
    data.append(STestData(QLatin1String("test66"), QByteArray(serializedChars3, sizeof(serializedChars3))));
    data.append(STestData(QLatin1String("7seven7"), QByteArray(serializedChars4, sizeof(serializedChars4))));

    for (int i = 0; i < data.count(); ++i) {
        QBuffer device;
        device.open(QBuffer::WriteOnly);

        CTelegramStream stream(&device);

        stream << data.at(i).value.toString();

        QCOMPARE(device.data(), data.at(i).serializedData);
    }

    for (int i = 0; i < data.count(); ++i) {
        QBuffer device;
        device.setData(data.at(i).serializedData);
        device.open(QBuffer::ReadOnly);

        CTelegramStream stream(&device);

        QString result;

        stream >> result;

        QCOMPARE(result, data.at(i).value.toString());
    }
}

void tst_CTelegramStream::stringsLimitSerialization()
{
    QList<STestData> data;

    QString stringToTest;
    QByteArray serializedString;

    for (int i = 0; i < 5; ++i) {
        stringToTest = QString(i, QChar('a'));
        serializedString = QByteArray(i, char('a'));
        serializedString.prepend(char(i));
        if (serializedString.length() % 4)
            serializedString.append(QByteArray(4 - serializedString.length() % 4, char(0)));

        data.append(STestData(stringToTest, serializedString));
    }

    stringToTest = QString(253, QChar::fromLatin1(char(0x70)));
    serializedString = QByteArray(253, char(0x70));
    serializedString.prepend(char(253));
    serializedString.append(char(0));
    serializedString.append(char(0));

    data.append(STestData(stringToTest, serializedString));

    stringToTest = QString(254, QChar::fromLatin1(char(0x71)));
    serializedString = QByteArray(254, char(0x71));

    serializedString.prepend(char(0)); // LengthBig
    serializedString.prepend(char(0)); // LengthMiddle
    serializedString.prepend(char(254)); // LengthLittle

    serializedString.prepend(char(254)); // Long-string marker

    serializedString.append(char(0)); // Padding1
    serializedString.append(char(0)); // Padding2

    data.append(STestData(stringToTest, serializedString));

    for (int i = 0; i < data.count(); ++i) {
        QBuffer device;
        device.open(QBuffer::WriteOnly);

        CTelegramStream stream(&device);

        stream << data.at(i).value.toString();

        if ((data.at(i).serializedData.length() > 10) && (device.data() != data.at(i).serializedData)) {
            qDebug() << QString("Actual (%1 bytes):").arg(device.data().length()) << device.data().toHex();
            qDebug() << QString("Expected (%1 bytes):").arg(data.at(i).serializedData.length()) << data.at(i).serializedData.toHex();
        }

        QCOMPARE(device.data(), data.at(i).serializedData);
    }

    for (int i = 0; i < data.count(); ++i) {
        QBuffer device;
        device.setData(data.at(i).serializedData);
        device.open(QBuffer::ReadOnly);

        CTelegramStream stream(&device);

        QString result;

        stream >> result;

        if (result != data.at(i).value.toString()) {
            qDebug() << QString("Error at %1").arg(i);
        }

        QCOMPARE(result, data.at(i).value.toString());
    }
}

void tst_CTelegramStream::longStringSerialization()
{
    QList<STestData> data;

    static const QString stringToTest1 = QStringLiteral("This is a long string, which is going to be serialized. "
                                                        "Starting to speek something wise and sage... Abadfa"
                                                        "dfa uzxcviuwekrhalflkjhzvzxciuvyoiuwehrkblabla-asdfasdf"
                                                        "qwrq25!@!@(#*&!$@%@!()#*&@#($)&*)(!@*&#)(!*@&#()!@*&$%!"
                                                        "*@&$&^!%$&*#@%$&*%^@#$&%@#$@#*$&^@#*(&^@#%*&^!F132465798"
                                                        "+760++-*/*/651321///???asd0f98`0978jhkjhzxcv....end");

    static const QString stringToTest2 = QStringLiteral("It seems that we need to test some much longer string"
                                                        "with four hundred characters at least. AS:DLMZXCoiu123787"
                                                        "Ljlkj123l4mzxcv989871234-09-0asd;lv,;,l;xzcvllas "
                                                        "  1234asplk;lkxzv==+_)!@#(*&234234zsdclkmasd2143164537xgcxn"
                                                        "  1234asplk;lkxzv==+_)!@#(*&234234zsdclkmasd2143164537xgcxn"
                                                        "  1234asplk;lkxzv==+_)!@#(*&234234zsdclkmasd2143164537xgcxn"
                                                        "  1234asplk;lkxzv==+_)!@#(*&234234zsdclkmasd2143164537xgcxn"
                                                        "  1234asplk;lkxzv==+_)!@#(*&234234zsdclkmasd2143164537xgcxn"
                                                        "  1234asplk;lkxzv==+_)!@#(*&234234zsdclkmasd2143164537xgcxn"
                                                        "  1234asplk;lkxzv==+_)!@#(*&234234zsdclkmasd2143164537xgcxn"
                                                        "  1234asplk;lkxzv==+_)!@#(*&234234zsdclkmasd2143164537xgcxn"
                                                        "here we are!"
                                                        );

    auto serialize = [](const QByteArray &str) {
        const int len = str.length();
        QByteArray serialized(1, char(0xfe));
        serialized.append(char(len & 0xff));
        serialized.append(char((len & 0xff00) >> 8));
        serialized.append(char((len & 0xff0000) >> 16));
        serialized.append(str);

        while (serialized.length() % 4) {
            serialized.append(char(0));
        }
        return serialized;
    };

    data.append(STestData(stringToTest1, serialize(stringToTest1.toUtf8())));
    data.append(STestData(stringToTest2, serialize(stringToTest2.toUtf8())));

    /* Serialization */
    for (int i = 0; i < data.count(); ++i) {
        QByteArray dataArray;
        CTelegramStream stream(&dataArray, true);
        const QString originString = data.at(i).value.toString();
        stream << originString;
        QVERIFY2(!(dataArray.length() % 4), "Results data is not padded (total length is not divisible by 4)");
        QVERIFY2((dataArray.length() >= originString.length() + 4), "Results data size for long string should be at least stringLength + one byte for 0xfe + three bytes for size");
        QVERIFY2((dataArray.length() <= originString.length() + 4 + 3), "Results data size for long string should never be more, than stringLength + 4 + 3,"
                                                                        "because we should never pad for more, than 3 bytes");
        QCOMPARE(dataArray, data.at(i).serializedData);
    }

    /* Deserialization */
    for (int i = 0; i < data.count(); ++i) {
        CTelegramStream stream(data.at(i).serializedData);
        QString result;
        stream >> result;
        QCOMPARE(result, data.at(i).value.toString());
    }
}

void tst_CTelegramStream::intSerialization()
{
    QList<STestData> data;

    {
        QByteArray testSerialized;
        quint32 values[5] = { 0x01, 0xff, 0x00, 0xaabbcc, 0xdeadbeef };
        const char serialized1[4] = { char(values[0]), 0, 0, 0 };
        const char serialized2[4] = { char(values[1]), 0, 0, 0 };
        const char serialized3[4] = { char(values[2]), 0, 0, 0 };
        const char serialized4[4] = { char(0xcc), char(0xbb), char(0xaa), char(0x00) };
        const char serialized5[4] = { char(0xef), char(0xbe), char(0xad), char(0xde) };

        testSerialized.append(serialized1, 4);
        testSerialized.append(serialized2, 4);
        testSerialized.append(serialized3, 4);
        testSerialized.append(serialized4, 4);
        testSerialized.append(serialized5, 5);

        for (int i = 0; i < 5; ++i)
            data.append(STestData(values[i], testSerialized.mid(i * 4, 4)));
    }

    for (int i = 0; i < data.count(); ++i) {
        QBuffer device;
        device.open(QBuffer::WriteOnly);

        CTelegramStream stream(&device);

        stream << data.at(i).value.value<quint32>();

        QCOMPARE(device.data(), data.at(i).serializedData);
    }

    for (int i = 0; i < data.count(); ++i) {
        QBuffer device;
        device.setData(data.at(i).serializedData);
        device.open(QBuffer::ReadOnly);

        CTelegramStream stream(&device);

        quint32 result;

        stream >> result;

        QCOMPARE(result, data.at(i).value.value<quint32>());
    }
}

void tst_CTelegramStream::vectorOfIntsSerialization()
{
    TLVector<quint64> vector;
    vector.append(0x12345678);
    QByteArray encoded = QByteArray::fromHex("15c4b51c010000007856341200000000");

    if (TLValue::Vector != 0x1cb5c415) {
        QFAIL("The test data is not updated to the current API yet");
    }

    {
        QByteArray output;
        CTelegramStream stream(&output, /* write */ true);
        stream << vector;
        QCOMPARE(output, encoded);
    }

    {
        CTelegramStream stream(encoded);
        TLVector<quint64> value;
        stream >> value;

        QCOMPARE(value, vector);
    }
}

void tst_CTelegramStream::vectorDeserializationError()
{
    TLVector<quint32> vector;

    const QByteArray encoded = QByteArray::fromHex("12345678");

    {
        CTelegramStream stream(encoded);

        stream >> vector;
        QVERIFY(!vector.isValid());
    }
}

void tst_CTelegramStream::pointerVectorSerialization()
{
    TLVector<quint32> values = { 1, 2, 3, 4, 5 };
    const TLVector<quint32*> writePtrs = {
        &values[0],
        &values[1],
        &values[2],
        &values[3],
        &values[4],
    };

    QByteArray buffer;
    {
        CTelegramStream stream(&buffer, true);
        stream << writePtrs;
    }
    QVERIFY(!buffer.isEmpty());

    CTelegramStream stream(buffer);
    TLVector<quint32> readValues;
    stream >> readValues;
    QCOMPARE(values.count(), readValues.count());
    for (int i = 0; i < values.count(); ++i) {
        QCOMPARE(values.at(i), readValues.at(i));
    }
}

void tst_CTelegramStream::pointerVectorDeserialization()
{
    const TLVector<quint32> writeValues = { 1, 2, 3, 4, 5 };

    QByteArray buffer;
    {
        CTelegramStream stream(&buffer, true);
        stream << writeValues;
    }
    QVERIFY(!buffer.isEmpty());

    CTelegramStream stream(buffer);
    TLVector<quint32*> readPtrs;
    stream >> readPtrs;
    QCOMPARE(writeValues.count(), readPtrs.count());
    for (int i = 0; i < writeValues.count(); ++i) {
        QCOMPARE(writeValues.at(i), *readPtrs.at(i));
    }
}

void tst_CTelegramStream::tlNumbersSerialization()
{
    QVector<TLNumber128> vector128;
    QVector<QByteArray> encoded128;

    TLNumber128 num128;
    QByteArray encoded;

    {
        num128.parts[0] = 1;
        num128.parts[1] = 0;
        encoded = QByteArray::fromHex("01000000000000000000000000000000");
        vector128.append(num128);
        encoded128.append(encoded);
    }
    {
        num128.parts[0] = 0;
        num128.parts[1] = 1;
        encoded = QByteArray::fromHex("00000000000000000100000000000000");
        vector128.append(num128);
        encoded128.append(encoded);
    }
    {
        num128.parts[0] = 0x00001000;
        num128.parts[1] = 0xdeadbeef;
        encoded = QByteArray::fromHex("0010000000000000efbeadde00000000");
        vector128.append(num128);
        encoded128.append(encoded);
    }
    {
        num128.parts[0] = 0x123456789abcdef0ull;
        num128.parts[1] = 0xdeadbeeff0011234ull;
        encoded = QByteArray::fromHex("f0debc9a78563412341201f0efbeadde");
        vector128.append(num128);
        encoded128.append(encoded);
    }

    for (int i = 0; i < vector128.count(); ++i) {
        QBuffer device;
        device.open(QBuffer::WriteOnly);

        CTelegramStream stream(&device);

        stream << vector128.at(i);
        QCOMPARE(device.data().toHex(), encoded128.at(i).toHex());
    }

    for (int i = 0; i < vector128.count(); ++i) {
        QBuffer device;
        device.setData(encoded128.at(i));
        device.open(QBuffer::ReadOnly);

        CTelegramStream stream(&device);

        TLNumber128 value;

        stream >> value;

        QCOMPARE(value, vector128.at(i));
    }
}

void tst_CTelegramStream::tlDcOptionDeserialization()
{
    QByteArray dcOptionsData;
    CTelegramStream inputStream(&dcOptionsData, /* write */ true);
    TLVector<TLDcOption> optionsVector;

    TLDcOption opt1;
    opt1.flags = 0;
    opt1.id = 2;
    opt1.ipAddress = QStringLiteral("1.2.3.4");
    opt1.port = 80;
    TLDcOption opt2;
    opt2.flags = 0;
    opt2.id = 3;
    opt2.ipAddress = QStringLiteral("2.3.4.5");
    opt2.port = 180;
    TLDcOption opt3;
    opt3.flags = 0;
    opt3.id = 4;
    opt3.ipAddress = QStringLiteral("3.4.5.6");
    opt3.port = 443;
    optionsVector << opt1 << opt2 << opt3;
    inputStream << optionsVector;

    CTelegramStream stream(dcOptionsData);
    TLVector<TLDcOption> readOptionsVector;
    stream >> readOptionsVector;

    QCOMPARE(readOptionsVector.size(), 3);
    QCOMPARE(readOptionsVector.at(0).ipAddress, opt1.ipAddress);
    QCOMPARE(readOptionsVector.at(1).ipAddress, opt2.ipAddress);
    QCOMPARE(readOptionsVector.at(2).ipAddress, opt3.ipAddress);

    QCOMPARE(readOptionsVector.at(0).flags, opt1.flags);
    QCOMPARE(readOptionsVector.at(0).port, opt1.port);
    QCOMPARE(readOptionsVector.at(1).id, opt2.id);
    QCOMPARE(readOptionsVector.at(2).id, opt3.id);

    QVERIFY(readOptionsVector.isValid());
}

void tst_CTelegramStream::readError()
{
    {
        static const char input[8] = { char(6), 't', 'e', 's', 't', '1', 'a', 0 };
        static const QByteArray data(input, sizeof(input));

        CTelegramStream stream(data);

        QVERIFY2(!stream.error(), "Unexpected error (there was been no read operation, so nothing can cause an error).");

        QString result;
        stream >> result;

        QCOMPARE(result, QLatin1String("test1a"));
        QVERIFY2(!stream.error(), "Stream is at the end of input, but without read it should be not an error yet.");

        // nothing to read
        stream >> result;
        QVERIFY2(stream.error(), "Read after the end should be an error.");
    }

    {
        static const char input[5] = { char(0xcc), char(0xbb), char(0xaa), char(0x00), char(0x20) };
        static const QByteArray data(input, sizeof(input));

        CTelegramStream stream(data);

        QVERIFY(!stream.error());

        quint32 result;
        stream >> result;

        QCOMPARE(result, quint32(0xaabbcc));
        QVERIFY2(!stream.error(), "Stream is at the end of input, but without read it should be not a error yet.");

        stream >> result;
        QVERIFY2(stream.error(), "Partial read, error should be set.");
    }

}

void tst_CTelegramStream::byteArrays()
{
    QByteArray output;
    CTelegramStream stream(&output, /* write */ true);
    QByteArray array1 = QByteArrayLiteral("array1");
    QByteArray array2 = QByteArrayLiteral("array2");

    stream << array1;
    stream << array2;

    CTelegramStream inputStream(output);
    QByteArray a1;
    QByteArray a2;
    inputStream >> a1;
    inputStream >> a2;
    QCOMPARE(array1, a1);
    QCOMPARE(array2, a2);
}

void tst_CTelegramStream::reqPqData()
{
    TLNumber128 clientNonce;
    clientNonce.parts[0] = 0x123456789abcdef0ull;
    clientNonce.parts[1] = 0xabcdefabc0011234ull;

    TLNumber128 serverNonce;
    serverNonce.parts[0] = 0x56781234abcdef00ull;
    serverNonce.parts[1] = 0xbcdefabcd0011224ull;

    quint64 fingerprint = 12345678ull;

    quint64 pq = 3;
    QByteArray pqAsByteArray(sizeof(pq), Qt::Uninitialized);
    qToBigEndian<quint64>(pq, (uchar *) pqAsByteArray.data());

    const TLVector<quint64> fingersprints = { fingerprint };

    QVector<int> bytes;
    QByteArray output;
    {
        CTelegramStream outputStream(&output, /* write */ true);
        outputStream << TLValue::ResPQ;
        bytes << output.size();
        outputStream << clientNonce;
        bytes << output.size();
        outputStream << serverNonce;
        bytes << output.size();
        outputStream << pqAsByteArray;
        bytes << output.size();
        outputStream << fingersprints;
        bytes << output.size();
    }

    CTelegramStream inputStream(output);
    {
        TLValue responsePqValue;
        inputStream >> responsePqValue;
        QCOMPARE(inputStream.bytesAvailable(), output.size() - bytes.at(0));
        QVERIFY2(responsePqValue == TLValue::ResPQ, "Error: Unexpected operation code");
    }

    {
        TLNumber128 nonce;
        inputStream >> nonce;
        QCOMPARE(inputStream.bytesAvailable(), output.size() - bytes.at(1));
        QVERIFY2(nonce == clientNonce, "Error: Read client nonce is different from the written own.");
    }

    {
        TLNumber128 nonce;
        inputStream >> nonce;
        QCOMPARE(inputStream.bytesAvailable(), output.size() - bytes.at(2));
        QVERIFY2(nonce == serverNonce, "Error: Read server nonce is different from the written own.");
    }

    {
        QByteArray pqData;
        inputStream >> pqData;
        QCOMPARE(inputStream.bytesAvailable(), output.size() - bytes.at(3));
        QCOMPARE(pqAsByteArray, pqData);

        quint64 convertedPq = qFromBigEndian<quint64>(reinterpret_cast<const uchar*>(pqData.constData()));
        QCOMPARE(convertedPq, pq);
    }

    {
        TLVector<quint64> fingersprints;
        inputStream >> fingersprints;
        QCOMPARE(inputStream.bytesAvailable(), output.size() - bytes.at(4));
        QCOMPARE(fingersprints.count(), 1);
        QCOMPARE(fingersprints.first(), fingerprint);
    }
}

void tst_CTelegramStream::testStreamView_viewLocksAndSkip()
{
    constexpr int dataSize = 32;
    char c_data[dataSize];
    CTelegramStream stream(QByteArray::fromRawData(c_data, dataSize));

    QCOMPARE(stream.bytesAvailable(), dataSize);
    {
        QVERIFY(!stream.isLocked());
        Telegram::Utils::IODevicePtr deviceView(stream.getDeviceView(dataSize / 2));
        QVERIFY(stream.isLocked());
        QCOMPARE(stream.bytesAvailable(), 0);
    }
    QVERIFY(!stream.isLocked());
    QCOMPARE(stream.bytesAvailable(), dataSize / 2);
}

void tst_CTelegramStream::testStreamView_viewTakeBytes()
{
    constexpr int dataSize = 32;
    constexpr int partSize = 8;
    QByteArray data = QByteArrayLiteral("0123456789ABCDEF").toHex();
    QVERIFY(data.size() == dataSize);
    CTelegramStream stream(data);

    QCOMPARE(stream.bytesAvailable(), dataSize);
    QByteArray firstPart = stream.readBytes(partSize);
    QCOMPARE(data.mid(0, partSize), firstPart);
    QCOMPARE(stream.bytesAvailable(), dataSize - partSize);
    {
        Telegram::Utils::IODevicePtr streamSubview(stream.getDeviceView(partSize * 2));
        CTelegramStream innerStream(streamSubview);
        QByteArray secondPart = innerStream.readBytes(partSize);
        QCOMPARE(data.mid(partSize, partSize), secondPart);
    }
    QByteArray lastPart = stream.readBytes(partSize);
    QCOMPARE(data.right(partSize), lastPart);
}

void tst_CTelegramStream::testStreamView_readOutOfView()
{
    constexpr int dataSize = 32;
    constexpr int partSize = 8;
    QByteArray data = QByteArrayLiteral("0123456789ABCDEF").toHex();
    QVERIFY(data.size() == dataSize);
    CTelegramStream stream(data);

    QCOMPARE(stream.bytesAvailable(), dataSize);
    QByteArray firstPart = stream.readBytes(partSize);
    QCOMPARE(data.mid(0, partSize), firstPart);
    QCOMPARE(stream.bytesAvailable(), dataSize - partSize);
    {
        Telegram::Utils::IODevicePtr streamSubview(stream.getDeviceView(partSize));
        CTelegramStream innerStream(streamSubview);
        QByteArray secondPart = innerStream.readBytes(partSize * 2);
        QCOMPARE(data.mid(partSize, partSize), secondPart);
        QVERIFY(innerStream.error());
    }
    QVERIFY(!stream.error());
    QByteArray lastPart = stream.readBytes(partSize * 2);
    QCOMPARE(data.right(partSize * 2), lastPart);
}

struct ViewSpec
{
    int offset;
    int size;
};
Q_DECLARE_METATYPE(ViewSpec)

using StreamView = QVector<ViewSpec>;
Q_DECLARE_METATYPE(StreamView)

void tst_CTelegramStream::testStreamView_benchmarkView_data()
{
    QTest::addColumn<int>("sourceSize");
    QTest::addColumn<StreamView>("view");

    QTest::newRow("0-16 in 64 bytes") << 64
                                      << StreamView({
                                                  { 0, 16 },
                                              });

    QTest::newRow("0-16 in 64 bytes") << 64
                                      << StreamView({
                                                  { 0, 16 },
                                                  { 0, 16 },
                                                  { 0, 16 },
                                                  { 0, 16 },
                                              });

    QTest::newRow("512 in 4096 bytes") << 4096
                                      << StreamView({
                                                  { 0, 512 },
                                                  { 0, 512 },
                                                  { 0, 512 },
                                                  { 0, 512 },
                                              });

    QTest::newRow("1024 in 4096 bytes") << 4096
                                      << StreamView({
                                                  { 0, 1024 },
                                                  { 0, 1024 },
                                                  { 0, 1024 },
                                                  { 0, 1024 },
                                              });
}

void tst_CTelegramStream::testStreamView_benchmarkView()
{
    QFETCH(int, sourceSize);
    QFETCH(const StreamView, view);
    const QByteArray sourceData(sourceSize, Qt::Uninitialized);

    //QBENCHMARK {

    QBENCHMARK {
        CTelegramStream stream(sourceData);
        for (const ViewSpec &spec : view) {
            stream.readBytes(spec.offset);
            Telegram::Utils::IODevicePtr streamSubview(stream.getDeviceView(spec.size));
            CTelegramStream innerStream(streamSubview);
            QCOMPARE(innerStream.bytesAvailable(), spec.size);
            for (int i = 0; i < spec.size / 4; ++i) {
                quint32 chunk;
                innerStream >> chunk;
            }
        }
    }
}

void tst_CTelegramStream::testStreamView_benchmarkCopy_data()
{
    return testStreamView_benchmarkView_data();
}

void tst_CTelegramStream::testStreamView_benchmarkCopy()
{
    QFETCH(int, sourceSize);
    QFETCH(const StreamView, view);
    const QByteArray sourceData(sourceSize, Qt::Uninitialized);

    QBENCHMARK {
        CTelegramStream stream(sourceData);
        for (const ViewSpec &spec : view) {
            const QByteArray viewData = stream.readBytes(spec.size);
            CTelegramStream innerStream(viewData);
            QCOMPARE(innerStream.bytesAvailable(), spec.size);
            for (int i = 0; i < spec.size / 4; ++i) {
                quint32 chunk;
                innerStream >> chunk;
            }
        }
    }
}

//QTEST_APPLESS_MAIN(tst_CTelegramStream)
QTEST_GUILESS_MAIN(tst_CTelegramStream)

#include "tst_CTelegramStream.moc"
