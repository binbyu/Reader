#pragma once

namespace IMG
{
    const unsigned char CRYPTO_KEY = 0x23;
    const unsigned int  BARCODE_ALIPAY_LENGHT = 74814;
    const unsigned int  BARCODE_WECHAT_LENGHT = 90926;
    const unsigned int  BARCODE_MP_LENGHT = 27151;

    extern const unsigned char* barcodeAlipay();
    extern const unsigned char* barcodeWechat();
    extern const unsigned char* barcodeMp();
}