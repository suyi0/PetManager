<template>
  <section class="home-page">
    <aside class="home-tabs">
      <button
        v-for="item in tabs"
        :key="item.key"
        class="home-tabs__item"
        :class="{ 'home-tabs__item--active': activeTab === item.key }"
        @click="switchTab(item.key)"
      >
        <span>{{ item.label }}</span>
        <small>{{ item.tagline }}</small>
      </button>
    </aside>

    <div class="home-stage">
      <section class="home-hero">
        <div class="home-hero__copy">
          <p>{{ currentContent.eyebrow }}</p>
          <h2>{{ currentContent.title }}</h2>
          <span>{{ currentContent.description }}</span>
        </div>
        <div
          class="home-hero__visual"
          :style="
            currentContent.image
              ? { backgroundImage: `url(${currentContent.image})` }
              : undefined
          "
        >
          <div class="home-hero__overlay">
            <strong>{{ currentContent.highlightTitle }}</strong>
            <small>{{ currentContent.highlightText }}</small>
          </div>
        </div>
      </section>

      <section class="home-grid">
        <article
          v-for="point in currentContent.points"
          :key="point.title"
          class="home-card"
        >
          <p>{{ point.label }}</p>
          <h3>{{ point.title }}</h3>
          <span>{{ point.text }}</span>
        </article>
      </section>

      <section v-if="activeTab === 'contact'" class="contact-panel">
        <div class="contact-panel__block">
          <p>电话</p>
          <strong>123-456-7890</strong>
          <span>工作日 09:00 - 20:00，节假日可留言回呼。</span>
        </div>
        <div class="contact-panel__block">
          <p>邮箱</p>
          <strong>care@petmanager.example</strong>
          <span>支持病例咨询、复诊预约、服务反馈。</span>
        </div>
        <div class="contact-panel__block">
          <p>到院地址</p>
          <strong>城市宠护大道 18 号</strong>
          <span>院内设独立候诊区、猫犬分区与夜间急诊接待台。</span>
        </div>
      </section>
    </div>
  </section>
</template>

<script setup lang="ts">
import { computed, ref } from "vue";
import backgroundImage from "@/assets/photo/background.jpeg";

type AboutTabKey =
  | "environment"
  | "doctor"
  | "machine"
  | "advantage"
  | "consultingService"
  | "contact";

const tabs: Array<{ key: AboutTabKey; label: string; tagline: string }> = [
  { key: "environment", label: "医院环境", tagline: "舒缓、整洁、清晰分区" },
  { key: "doctor", label: "医生团队", tagline: "专科协作与接诊经验" },
  { key: "machine", label: "设备优势", tagline: "检查效率与成像能力" },
  { key: "advantage", label: "服务优势", tagline: "从候诊到复诊都更顺畅" },
  {
    key: "consultingService",
    label: "免费咨询",
    tagline: "先问再约，少走弯路",
  },
  { key: "contact", label: "联系我们", tagline: "电话、邮箱与到院方式" },
];

const activeTab = ref<AboutTabKey>("environment");

const contentMap: Record<
  AboutTabKey,
  {
    eyebrow: string;
    title: string;
    description: string;
    highlightTitle: string;
    highlightText: string;
    image?: string;
    points: Array<{ label: string; title: string; text: string }>;
  }
> = {
  environment: {
    eyebrow: "Environment",
    title: "更像精品宠护会所，而不是传统拥挤门诊。",
    description:
      "候诊区、咨询区、检查区之间清楚分流，猫犬动线尽量分开，让宠物和主人都更放松。",
    highlightTitle: "温和的动线设计",
    highlightText: "从入院登记到离院结算，尽量减少反复排队与来回折返。",
    image: backgroundImage,
    points: [
      {
        label: "候诊",
        title: "安静分区",
        text: "犬区与猫区分开布置，减少陌生气味与声音带来的焦虑。",
      },
      {
        label: "诊室",
        title: "隐私感更好",
        text: "问诊区与配药区拆分，沟通时更专注，不容易被打断。",
      },
      {
        label: "照护",
        title: "消毒流程透明",
        text: "重点区域有明确清洁频次，日常可见，使用也更放心。",
      },
    ],
  },
  doctor: {
    eyebrow: "Team",
    title: "医生、护理与前台协同处理，让接诊节奏更顺。",
    description:
      "用户端会优先展示可预约医生与时段，减少信息不对称带来的等待和反复确认。",
    highlightTitle: "多岗位联动",
    highlightText: "初诊、复诊、术后回访的信息链尽量不断层。",
    points: [
      {
        label: "内科",
        title: "常规诊疗经验丰富",
        text: "覆盖体检、肠胃、皮肤、慢病管理等高频需求。",
      },
      {
        label: "外科",
        title: "术前沟通更细",
        text: "把术前检查、费用区间与恢复节点提前讲清楚。",
      },
      {
        label: "护理",
        title: "术后回访机制",
        text: "围绕喂药、恢复表现与复诊提醒给出连续反馈。",
      },
    ],
  },
  machine: {
    eyebrow: "Equipment",
    title: "检查设备更聚焦“效率”和“解释清楚”。",
    description:
      "不是单纯堆机器，而是让常规检查尽快完成，并能把结果说得明白、可追溯。",
    highlightTitle: "快速检查链路",
    highlightText: "高频项目尽量集中安排，减少重复等待。",
    points: [
      {
        label: "影像",
        title: "成像信息更完整",
        text: "用于常见骨科、胸腹部检查时能更快给出初步判断。",
      },
      {
        label: "检验",
        title: "基础项目可快速回传",
        text: "让化验、问诊和治疗建议形成连续链路。",
      },
      {
        label: "留档",
        title: "复诊对比更方便",
        text: "关键检查结果可用于后续病程跟踪与方案调整。",
      },
    ],
  },
  advantage: {
    eyebrow: "Service",
    title: "把复杂流程整理成容易理解的服务节奏。",
    description:
      "用户不用先懂宠物医疗流程，也能快速知道自己下一步该做什么、在哪里看进度。",
    highlightTitle: "少解释成本",
    highlightText: "从预约、到院、订单，再到个人资料都尽量统一入口。",
    points: [
      {
        label: "预约",
        title: "时段选择清楚",
        text: "先选医生，再看日期和时段，路径更直接。",
      },
      {
        label: "订单",
        title: "记录集中管理",
        text: "订单和预约记录都能统一检索，不用反复翻聊天记录。",
      },
      {
        label: "资料",
        title: "个人信息随时维护",
        text: "地址、手机号、生日等资料可以独立更新。",
      },
    ],
  },
  consultingService: {
    eyebrow: "Consulting",
    title: "先咨询，再决定是否来院，减少无效奔波。",
    description:
      "对轻微症状、复诊判断、术后护理疑问，先给一个可执行的初步方向，再安排预约。",
    highlightTitle: "线上预沟通",
    highlightText: "问题提前梳理，到院后的问诊会更快进入重点。",
    points: [
      {
        label: "初筛",
        title: "先判断紧急程度",
        text: "帮助用户分辨是否需要立即到院处理。",
      },
      {
        label: "准备",
        title: "提前准备材料",
        text: "把既往检查、用药情况、症状变化先整理好。",
      },
      {
        label: "复诊",
        title: "减少重复问答",
        text: "回访问题提前归档，复诊时更快进入诊疗阶段。",
      },
    ],
  },
  contact: {
    eyebrow: "Contact",
    title: "把联系入口放在容易找到的位置。",
    description:
      "无论是预约前咨询、到院路线、还是术后回访需求，都能快速找到对应入口。",
    highlightTitle: "直接联系",
    highlightText: "适合处理服务咨询、预约确认、到院协助等高频需求。",
    points: [
      {
        label: "热线",
        title: "接待响应更快",
        text: "到院前咨询、预约确认、复诊提醒都可通过热线沟通。",
      },
      {
        label: "邮件",
        title: "适合留档需求",
        text: "病例资料、建议反馈、复杂问题可通过邮件整理发送。",
      },
      {
        label: "到院",
        title: "路线说明清楚",
        text: "方便首次来院用户快速找到前台与停车区域。",
      },
    ],
  },
};

const currentContent = computed(() => contentMap[activeTab.value]);

const switchTab = (tab: AboutTabKey) => {
  activeTab.value = tab;
};
</script>

<style scoped lang="scss">
.home-page {
  display: grid;
  grid-template-columns: 250px minmax(0, 1fr);
  gap: 20px;
  min-height: calc(100vh - 170px);
}

.home-tabs {
  position: sticky;
  top: 128px;
  align-self: start;
  display: grid;
  gap: 10px;
  padding: 18px;
  border-radius: 30px;
  border: 1px solid rgba(21, 91, 92, 0.1);
  background: rgba(255, 250, 242, 0.8);
  box-shadow: 0 22px 50px rgba(29, 93, 95, 0.08);
}

.home-tabs__item {
  display: grid;
  justify-items: start;
  gap: 4px;
  padding: 14px 16px;
  border: 1px solid transparent;
  border-radius: 20px;
  background: rgba(255, 255, 255, 0.58);
  color: #163f42;
  cursor: pointer;
  text-align: left;
}

.home-tabs__item span {
  font-size: 15px;
  font-weight: 700;
}

.home-tabs__item small {
  color: #67817d;
  font-size: 12px;
}

.home-tabs__item--active {
  border-color: rgba(29, 134, 135, 0.18);
  background: linear-gradient(
    135deg,
    rgba(136, 214, 206, 0.28),
    rgba(243, 197, 155, 0.22)
  );
  box-shadow: 0 16px 30px rgba(28, 98, 99, 0.1);
}

.home-stage {
  display: grid;
  gap: 18px;
}

.home-hero {
  display: grid;
  grid-template-columns: minmax(0, 1fr) 360px;
  gap: 18px;
  padding: 26px;
  border-radius: 34px;
  border: 1px solid rgba(21, 91, 92, 0.1);
  background: rgba(255, 250, 242, 0.82);
  box-shadow: 0 24px 55px rgba(25, 92, 93, 0.08);
}

.home-hero__copy {
  display: grid;
  align-content: start;
  gap: 12px;
}

.home-hero__copy p {
  margin: 0;
  color: #1e8a88;
  letter-spacing: 0.1em;
  text-transform: uppercase;
  font-size: 12px;
  font-weight: 700;
}

.home-hero__copy h2 {
  margin: 0;
  font-family: "ZCOOL XiaoWei", "Noto Serif SC", serif;
  font-size: clamp(34px, 4vw, 48px);
  line-height: 1.12;
  color: #143d40;
}

.home-hero__copy span {
  max-width: 760px;
  color: #5d7875;
  font-size: 15px;
  line-height: 1.8;
}

.home-hero__visual {
  position: relative;
  min-height: 280px;
  border-radius: 28px;
  background: linear-gradient(
      145deg,
      rgba(140, 214, 207, 0.9),
      rgba(24, 111, 112, 0.82)
    ),
    center / cover no-repeat;
  overflow: hidden;
}

.home-hero__overlay {
  position: absolute;
  left: 20px;
  right: 20px;
  bottom: 20px;
  padding: 18px 20px;
  border-radius: 22px;
  background: rgba(255, 248, 240, 0.84);
  color: #154245;
}

.home-hero__overlay strong {
  display: block;
  font-size: 18px;
  margin-bottom: 4px;
}

.home-hero__overlay small {
  color: #5f7774;
  font-size: 13px;
}

.home-grid {
  display: grid;
  grid-template-columns: repeat(3, minmax(0, 1fr));
  gap: 16px;
}

.home-card,
.contact-panel__block {
  padding: 22px;
  border-radius: 28px;
  border: 1px solid rgba(21, 91, 92, 0.08);
  background: rgba(255, 255, 255, 0.62);
  box-shadow: 0 20px 45px rgba(25, 92, 93, 0.07);
}

.home-card p,
.contact-panel__block p {
  margin: 0 0 10px;
  color: #1c8b89;
  font-size: 12px;
  font-weight: 700;
  letter-spacing: 0.08em;
  text-transform: uppercase;
}

.home-card h3,
.contact-panel__block strong {
  display: block;
  margin: 0 0 10px;
  font-size: 22px;
  color: #153f42;
}

.home-card span,
.contact-panel__block span {
  color: #5f7875;
  font-size: 14px;
  line-height: 1.8;
}

.contact-panel {
  display: grid;
  grid-template-columns: repeat(3, minmax(0, 1fr));
  gap: 16px;
}

@media (max-width: 1100px) {
  .home-page,
  .home-hero,
  .contact-panel,
  .home-grid {
    grid-template-columns: 1fr;
  }

  .home-tabs {
    position: static;
  }
}
</style>
