// utils/phoneValidator.ts
export const PhoneValidationRules = {
  // 中国手机号验证规则
  CHINA: /^1[3-9]\d{9}$/,
  
  // 美国手机号验证规则
  USA: /^(\+1|1)?[2-9]\d{2}[2-9]\d{2}\d{4}$/,
  
  // 国际标准 E.164 格式
  INTERNATIONAL: /^\+?[1-9]\d{1,14}$/
};

export class PhoneValidator {
  static validate(phone: string, country: string = 'INTERNATIONAL'): boolean {
    const rule = PhoneValidationRules[country as keyof typeof PhoneValidationRules];
    if (!rule) {
      throw new Error(`Unsupported country: ${country}`);
    }
    return rule.test(phone);
  }
  
  static format(phone: string, country: string = 'INTERNATIONAL'): string {
    // 格式化手机号码
    switch (country) {
      case 'CHINA':
        // 添加中国区号
        if (!phone.startsWith('+86')) {
          return '+86' + phone.replace(/\D/g, '');
        }
        return phone;
        
      case 'USA':
        // 添加美国区号
        if (!phone.startsWith('+1')) {
          return '+1' + phone.replace(/\D/g, '');
        }
        return phone;
        
      default:
        // 国际格式
        if (!phone.startsWith('+')) {
          return '+' + phone.replace(/\D/g, '');
        }
        return phone;
    }
  }
}