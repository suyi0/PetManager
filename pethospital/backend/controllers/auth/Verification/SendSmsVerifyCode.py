import os
import sys
import json
import argparse
import logging
from typing import Dict, Any

from alibabacloud_dypnsapi20170525.client import Client as Dypnsapi20170525Client
from alibabacloud_credentials.client import Client as CredentialClient
from alibabacloud_tea_openapi import models as open_api_models
from alibabacloud_dypnsapi20170525 import models as dypnsapi_20170525_models
from alibabacloud_tea_util import models as util_models
from alibabacloud_tea_util.client import Client as UtilClient

# 配置日志
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

class SmsSender:
    def __init__(self):
        """初始化SMS发送器"""
        self.client = self._create_client()
        
    def _create_client(self) -> Dypnsapi20170525Client:
        """创建阿里云客户端"""
        try:
            credential = CredentialClient()
            config = open_api_models.Config(credential=credential)
            config.endpoint = 'dypnsapi.aliyuncs.com'
            return Dypnsapi20170525Client(config)
        except Exception as e:
            logger.error(f"Failed to create client: {e}")
            raise
    
    def send_verification_code(self, phone_number: str, verify_code: str) -> Dict[str, Any]:
        """
        发送短信验证码
        
        Args:
            phone_number: 手机号码
            verify_code: 验证码
            
        Returns:
            包含发送结果的字典
        """
        try:
            # 获取环境变量配置，如果没有则使用默认值
            sign_name = os.getenv('ALIYUN_SMS_SIGN_NAME', '速通互联验证码')
            template_code = os.getenv('ALIYUN_SMS_TEMPLATE_CODE', '100001')
            
            # 构建模板参数
            template_param = json.dumps({"code": verify_code})
            
            # 创建请求对象
            request = dypnsapi_20170525_models.SendSmsVerifyCodeRequest(
                phone_number=phone_number,
                sign_name=sign_name,
                template_code=template_code,
                template_param=template_param,
                country_code='86',
                interval=60,        # 间隔60秒
                valid_time=300,     # 有效时间300秒
                return_verify_code=False,
                code_type=0
            )
            
            # 发送请求
            runtime = util_models.RuntimeOptions()
            response = self.client.send_sms_verify_code_with_options(request, runtime)
            
            logger.info(f"SMS sent successfully to {phone_number}")
            
            return {
                "success": True,
                "message": "验证码发送成功",
                "request_id": response.body.request_id if hasattr(response.body, 'request_id') else None,
                "code": response.body.code if hasattr(response.body, 'code') else None
            }
            
        except Exception as error:
            error_msg = str(error)
            error_code = getattr(error, 'code', 'UNKNOWN_ERROR')
            logger.error(f"Failed to send SMS: {error_msg}, code: {error_code}")
            
            return {
                "success": False,
                "message": error_msg,
                "error_code": error_code
            }

def main():
    """主函数 - 命令行接口"""
    parser = argparse.ArgumentParser(description='发送短信验证码')
    parser.add_argument('phone', help='手机号码')
    parser.add_argument('code', help='验证码')
    parser.add_argument('--json', action='store_true', help='以JSON格式输出结果')
    parser.add_argument('--debug', action='store_true', help='启用调试模式')
    
    args = parser.parse_args()
    
    # 设置日志级别
    if args.debug:
        logging.getLogger().setLevel(logging.DEBUG)
    
    try:
        # 发送验证码
        sender = SmsSender()
        result = sender.send_verification_code(args.phone, args.code)
        
        # 输出结果
        if args.json:
            print(json.dumps(result, ensure_ascii=False, indent=2))
        else:
            if result["success"]:
                print(f"✅ 验证码已发送到 {args.phone}")
            else:
                print(f"❌ 发送失败: {result['message']}")
                if 'error_code' in result:
                    print(f"错误代码: {result['error_code']}")
        
        # 返回退出码
        sys.exit(0 if result["success"] else 1)
        
    except Exception as e:
        logger.error(f"Unexpected error: {e}")
        if args.json:
            error_result = {
                "success": False,
                "message": str(e),
                "error_code": "INTERNAL_ERROR"
            }
            print(json.dumps(error_result, ensure_ascii=False, indent=2))
        else:
            print(f"❌ 系统错误: {e}")
        sys.exit(1)

if __name__ == '__main__':
    main()