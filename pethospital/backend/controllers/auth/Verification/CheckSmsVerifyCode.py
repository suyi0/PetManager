import os
import sys

from alibabacloud_dypnsapi20170525.client import Client as Dypnsapi20170525Client
from alibabacloud_tea_openapi import models as open_api_models
from alibabacloud_dypnsapi20170525 import models as dypnsapi_20170525_models
from alibabacloud_tea_util import models as util_models
from alibabacloud_tea_util.client import Client as UtilClient


def create_client(
    access_key_id,
    access_key_secret,
):
    config = open_api_models.Config(
        # 必填，您的 AccessKey ID,
        access_key_id=access_key_id,
        # 必填，您的 AccessKey Secret,
        access_key_secret=access_key_secret
    )
    # 访问的域名
    config.endpoint = f'dypnsapi.aliyuncs.com'
    return Dypnsapi20170525Client(config)


def main(
    args,
):
    client = create_client(os.environ['ALIBABA_CLOUD_ACCESS_KEY_ID'], os.environ['ALIBABA_CLOUD_ACCESS_KEY_SECRET'])
    check_sms_verify_code_request = dypnsapi_20170525_models.CheckSmsVerifyCodeRequest(
        case_auth_policy=0,
        country_code='cn',
        out_id='your_out_id',
        phone_number='your_phone_number',
        verify_code='your_verify_code'
    )
    runtime = util_models.RuntimeOptions()
    try:
        # 复制代码运行请自行打印 API 的返回值
        resp = client.check_sms_verify_code_with_options(check_sms_verify_code_request, runtime)
        print(resp)
    except Exception as error:
        # 如有需要，请打印 error
        UtilClient.assert_as_string(error.message)


async def main_async(
    args,
):
    client = create_client(os.environ['ALIBABA_CLOUD_ACCESS_KEY_ID'], os.environ['ALIBABA_CLOUD_ACCESS_KEY_SECRET'])
    check_sms_verify_code_request = dypnsapi_20170525_models.CheckSmsVerifyCodeRequest(
        case_auth_policy=0,
        country_code='cn',
        out_id='your_out_id',
        phone_number='your_phone_number',
        verify_code='your_verify_code'
    )
    runtime = util_models.RuntimeOptions()
    try:
        # 复制代码运行请自行打印 API 的返回值
        resp = await client.check_sms_verify_code_with_options_async(check_sms_verify_code_request, runtime)
        print(resp)
    except Exception as error:
        # 如有需要，请打印 error
        UtilClient.assert_as_string(error.message)


if __name__ == '__main__':
    main(sys.argv[1:])