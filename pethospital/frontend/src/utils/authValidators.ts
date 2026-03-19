export const PhoneValidationRules = {
  CHINA: /^1[3-9]\d{9}$/,
  USA: /^(\+1|1)?[2-9]\d{2}[2-9]\d{2}\d{4}$/,
  INTERNATIONAL: /^\+?[1-9]\d{1,14}$/,
} as const;

export const EMAIL_REGEX = /^[^\s@]+@[^\s@]+\.[^\s@]+$/;

export class PhoneValidator {
  static validate(
    phone: string,
    country: keyof typeof PhoneValidationRules = "INTERNATIONAL"
  ): boolean {
    const rule = PhoneValidationRules[country];
    return rule.test(phone);
  }

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

export const isEmail = (value: string) => {
  return EMAIL_REGEX.test(value);
};

export const isPhone = (value: string) => {
  const normalizedPhone = value.replace(/^\+86/, "");
  return PhoneValidationRules.CHINA.test(normalizedPhone);
};
