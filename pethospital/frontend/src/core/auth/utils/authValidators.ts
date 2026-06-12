/**
 * 手机号校验规则集合。
 * CHINA 用于国内 11 位手机号，USA 用于美国号码，INTERNATIONAL 用于 E.164 风格的国际号码。
 */
export const PhoneValidationRules = {
  CHINA: /^1[3-9]\d{9}$/,
  USA: /^(\+1|1)?[2-9]\d{2}[2-9]\d{2}\d{4}$/,
  INTERNATIONAL: /^\+?[1-9]\d{1,14}$/,
} as const;

/**
 * 邮箱基础格式校验规则。
 * 这里只做前端输入格式判断，真实邮箱归属仍以后端验证码校验为准。
 */
export const EMAIL_REGEX = /^[^\s@]+@[^\s@]+\.[^\s@]+$/;

/**
 * 手机号校验和格式化工具。
 */
export class PhoneValidator {
  /**
   * 校验手机号是否符合指定国家或国际号码规则。
   * @param phone 用户输入的手机号
   * @param country 校验规则，默认使用国际号码规则
   * @returns true 表示手机号格式符合对应规则
   */
  static validate(
    phone: string,
    country: keyof typeof PhoneValidationRules = "INTERNATIONAL"
  ): boolean {
    const rule = PhoneValidationRules[country];
    return rule.test(phone);
  }

  /**
   * 按指定国家规则补齐区号并去除非数字字符。
   * @param phone 用户输入的手机号
   * @param country 格式化规则，默认使用国际号码规则
   * @returns 带国家区号的手机号字符串
   */
  static format(
    phone: string,
    country: keyof typeof PhoneValidationRules = "INTERNATIONAL"
  ): string {
    switch (country) {
      case "CHINA":
        if (!phone.startsWith("+86")) {
          return "+86" + phone.replace(/\D/g, "");
        }
        return phone;
      case "USA":
        if (!phone.startsWith("+1")) {
          return "+1" + phone.replace(/\D/g, "");
        }
        return phone;
      default:
        if (!phone.startsWith("+")) {
          return "+" + phone.replace(/\D/g, "");
        }
        return phone;
    }
  }
}

/**
 * 判断输入值是否是基础邮箱格式。
 * @param value 用户输入的邮箱
 * @returns true 表示邮箱格式可进入下一步验证码流程
 */
export const isEmail = (value: string) => {
  return EMAIL_REGEX.test(value);
};

/**
 * 判断输入值是否是中国大陆手机号。
 * @param value 用户输入的手机号，可带 +86 前缀
 * @returns true 表示手机号符合国内 11 位手机号规则
 */
export const isPhone = (value: string) => {
  const normalizedPhone = value.replace(/^\+86/, "");
  return PhoneValidationRules.CHINA.test(normalizedPhone);
};
