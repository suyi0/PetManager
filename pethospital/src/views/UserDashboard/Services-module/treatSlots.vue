<template>
  <div class="reservation-slots">
    <!-- 关闭按钮 -->
    <div class="close-button" @click="close">
      <svg
        width="18"
        height="18"
        viewBox="0 0 18 18"
        fill="none"
        xmlns="http://www.w3.org/2000/svg"
      >
        <path
          d="M14 2L2 14M14 14L2 2"
          stroke="#333"
          stroke-width="2"
          stroke-linecap="round"
          stroke-linejoin="round"
        />
      </svg>
    </div>
    <div
      v-if="props.activeTab === 'reservation-treatSlots'"
      class="reservation-slots-page"
    >
      <div class="reservation-slots-top">
        <div class="title-container">
          <span>选择医生</span>
        </div>
      </div>
      <div class="reservation-slots-middle">
        <div
          v-for="doctor in doctorData"
          :key="doctor.id"
          class="reservation-slots-middle-div"
        >
          <img
            class="reservation-slots-middle-div-img"
            src="@/assets/photo/background.jpeg"
            alt="头像"
          />
          <span class="reservation-slots-middle-div-doctorName">{{
            doctor.name
          }}</span>
          <span class="reservation-slots-middle-div-span">医生介绍:</span>
          <button
            class="reservation-slots-middle-div-button"
            @click="
              props.switchTab('showSlots');
              choiceDoctor(doctor);
            "
          >
            <span>预约</span>
          </button>
        </div>
      </div>
    </div>
    <div v-if="props.activeTab === 'showSlots'" class="reservation-slots-page">
      <div class="reservation-slots-top">
        <div class="title-container">
          <span>预约时间</span>
        </div>
      </div>
      <div class="reservation-slots-middle">
        <div class="date-selectors">
          <div class="date-selectors-top">
            <span class="date-selectors-top-button-text">{{ year[0] }}年 </span>
            <div class="date-selectors-top-button-container">
              <div class="date-selectors-top-button-div">
                <button
                  @click="switchDate('date1')"
                  class="date-selectors-top-button1"
                  :class="{
                    ' click ': dateTab === 'date1',
                  }"
                >
                  <span class="date-selectors-top-button-text">
                    {{ weekday[0] }}
                  </span>
                  <span class="date-selectors-top-button-text"
                    >{{ month[0] }}月{{ day[0] }}日</span
                  >
                </button>
              </div>
              <div class="date-selectors-top-button-div">
                <button
                  @click="switchDate('date2')"
                  class="date-selectors-top-button2"
                  :class="{
                    ' click ': dateTab === 'date2',
                  }"
                >
                  <span class="date-selectors-top-button-text">
                    {{ weekday[1] }}
                  </span>
                  <span class="date-selectors-top-button-text"
                    >{{ month[1] }}月{{ day[1] }}日</span
                  >
                </button>
              </div>
              <div class="date-selectors-top-button-div">
                <button
                  @click="switchDate('date3')"
                  class="date-selectors-top-button3"
                  :class="{
                    ' click ': dateTab === 'date3',
                  }"
                >
                  <span class="date-selectors-top-button-text">
                    {{ weekday[2] }}
                  </span>
                  <span class="date-selectors-top-button-text"
                    >{{ month[2] }}月{{ day[2] }}日</span
                  >
                </button>
              </div>
              <div class="date-selectors-top-button-div">
                <button
                  @click="switchDate('date4')"
                  class="date-selectors-top-button4"
                  :class="{
                    ' click ': dateTab === 'date4',
                  }"
                >
                  <span class="date-selectors-top-button-text">
                    {{ weekday[3] }}
                  </span>
                  <span class="date-selectors-top-button-text"
                    >{{ month[3] }}月{{ day[3] }}日</span
                  >
                </button>
              </div>
              <div class="date-selectors-top-button-div">
                <button
                  @click="switchDate('date5')"
                  class="date-selectors-top-button5"
                  :class="{
                    ' click ': dateTab === 'date5',
                  }"
                >
                  <span class="date-selectors-top-button-text">
                    {{ weekday[4] }}
                  </span>
                  <span class="date-selectors-top-button-text"
                    >{{ month[4] }}月{{ day[4] }}日</span
                  >
                </button>
              </div>
              <div class="date-selectors-top-button-div">
                <button
                  @click="switchDate('date6')"
                  class="date-selectors-top-button6"
                  :class="{
                    ' click ': dateTab === 'date6',
                  }"
                >
                  <span class="date-selectors-top-button-text">
                    {{ weekday[5] }}
                  </span>
                  <span class="date-selectors-top-button-text"
                    >{{ month[5] }}月{{ day[5] }}日</span
                  >
                </button>
              </div>
              <div class="date-selectors-top-button-div">
                <button
                  @click="switchDate('date7')"
                  class="date-selectors-top-button7"
                  :class="{
                    ' click ': dateTab === 'date7',
                  }"
                >
                  <span class="date-selectors-top-button-text">
                    {{ weekday[6] }}
                  </span>
                  <span class="date-selectors-top-button-text"
                    >{{ month[6] }}月{{ day[6] }}日</span
                  >
                </button>
              </div>
            </div>
          </div>
          <div class="date-selectors-middle">
            <div class="date-selectors-middle-slots">
              <div class="date-selectors-middle-slots-morning">
                <span class="date-selectors-middle-slots-title">上午</span>
                <div class="date-selectors-middle-slots-morning-container">
                  <div
                    class="date-selectors-middle-slots-morning-container-left"
                  >
                    <span
                      v-for="(item, index) in getSlotArray(dateTab, 'morning')"
                      :key="'morning-time-' + dateTab + '-' + index"
                      >{{ item }}
                      <div
                        class="date-selectors-middle-slots-morning-container-right"
                      >
                        <button
                          class="date-selectors-middle-slots-morning-container-right-button"
                          @click="
                            switchChoice(
                              index === 0
                                ? 'button1'
                                : index === 1
                                ? 'button2'
                                : 'button3'
                            )
                          "
                        >
                          <img
                            v-if="
                              (index === 0 && choiceActive === 'button1') ||
                              (index === 1 && choiceActive === 'button2') ||
                              (index === 2 && choiceActive === 'button3')
                            "
                            src="@/assets/photo/钩.svg"
                            alt="钩"
                            class="date-selectors-middle-slots-morning-container-right-img"
                          />
                          <img
                            v-else
                            src="@/assets/photo/未选中.svg"
                            alt="未选中"
                            class="date-selectors-middle-slots-morning-container-right-img"
                          />
                        </button>
                      </div>
                    </span>
                  </div>
                </div>
              </div>
              <div class="date-selectors-middle-slots-afternoon">
                <span class="date-selectors-middle-slots-title">下午</span>
                <div class="date-selectors-middle-slots-afternoon-container">
                  <div
                    class="date-selectors-middle-slots-afternoon-container-left"
                  >
                    <span
                      v-for="(item, index) in getSlotArray(
                        dateTab,
                        'afternoon'
                      )"
                      :key="'afternoon-time-' + dateTab + '-' + index"
                      >{{ item }}
                      <div
                        class="date-selectors-middle-slots-afternoon-container-right"
                      >
                        <button
                          class="date-selectors-middle-slots-morning-container-right-button"
                          @click="
                            switchChoice(
                              index === 0
                                ? 'button4'
                                : index === 1
                                ? 'button5'
                                : index === 2
                                ? 'button6'
                                : 'button7'
                            )
                          "
                        >
                          <img
                            v-if="
                              (index === 0 && choiceActive === 'button4') ||
                              (index === 1 && choiceActive === 'button5') ||
                              (index === 2 && choiceActive === 'button6') ||
                              (index === 3 && choiceActive === 'button7')
                            "
                            src="@/assets/photo/钩.svg"
                            alt="钩"
                            class="date-selectors-middle-slots-morning-container-right-img"
                          />
                          <img
                            v-else
                            src="@/assets/photo/未选中.svg"
                            alt="未选中"
                            class="date-selectors-middle-slots-morning-container-right-img"
                          />
                        </button>
                      </div>
                    </span>
                  </div>
                </div>
              </div>
            </div>
          </div>
        </div>
      </div>
      <div class="reservation-slots-bottom">
        <div class="reservation-slots-bottom-container">
          <button
            class="reservation-slots-bottom-container-cancle"
            @click="cancle"
          >
            取消
          </button>
          <button
            class="reservation-slots-bottom-container-submit"
            @click="submit(dateTab, choiceActive)"
          >
            确定
          </button>
        </div>
      </div>
      <div
        v-if="submitAfter"
        class="reservation-slots-bottom-container-submit-after"
      >
        <h3>预约成功</h3>
        <p style="margin-bottom: 20px">
          您已成功预约 {{ upYear }}-{{ upMonth }}-{{ upDay }}
          {{ upSlot }}
        </p>
        <button
          class="reservation-slots-bottom-container-submit-after-button"
          @click="removeSubmitAfter"
        >
          确定
        </button>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
// 1. 导入部分
import { ref, onMounted, onBeforeUnmount, computed } from "vue";
import { useStore } from "vuex";
import { key } from "@/store/userStore";
// import { useRouter, useRoute } from "vue-router";
// import type { PropType } from "vue";

const store = useStore(key);

// 2. Props定义 (如果需要接收父组件传递的数据)
const props = defineProps<{
  activeTab: string;
  switchTab(_tab: string): void;
}>();

// 3. Emit事件定义 (如果需要向父组件传递事件)
const emit = defineEmits(["close", "cancle", "submit-success"]);

// 4. 响应式数据
interface Doctor {
  id: number;
  name: string;
}

const doctorData = computed<Doctor[]>(() => {
  const data = store.state.auth.reservate.doctorData;
  // 如果是单个对象，转换为数组；如果是数组，直接返回
  return Array.isArray(data) ? data : [data];
});
const dateTab = ref("date1");
const choiceActive = ref("");
const year = computed(() => store.state.auth.reservate.year);
const month = computed(() => store.state.auth.reservate.month);
const day = computed(() => store.state.auth.reservate.day);
const weekday = computed(() => store.state.auth.reservate.weekday);
const slots = computed(() => store.state.auth.reservate.slots);
const upDoctorId = ref<number>(0); //上传的医生ID
const upYear = ref(""); //上传的年份
const upMonth = ref(""); //上传的月份
const upDay = ref(""); //上传的日期
const upSlot = ref(""); //上传的时间段
const submitAfter = ref(false);

// 5. 计算属性

// 6. 方法定义
function close() {
  emit("close");
}

function cancle() {
  emit("cancle");
}

function choiceDoctor(doctor: Doctor) {
  if (upDoctorId.value !== 0) {
    upDoctorId.value = 0; // 重置医生ID
  }
  upDoctorId.value = doctor.id;
}

function getSlotArray(
  dateTab: string,
  period: "morning" | "afternoon"
): string[] {
  const slotIndex = judgehDate(dateTab);
  let choiceMorningSlots = 0;
  for (let i = 0; i < slots.value[slotIndex].length; ++i) {
    if (slots.value[slotIndex][i].substr(6, 5) === "12:00") {
      choiceMorningSlots = i + 1;
      break;
    }
  }
  // 确保 slots.value 和 slots.value[slotIndex] 存在
  if (slots.value && slots.value[slotIndex]) {
    // 上午时段返回前3个元素
    if (period === "morning") {
      const morningSlots = slots.value[slotIndex].slice(0, choiceMorningSlots);
      // 确保返回的数组始终有3个元素
      while (morningSlots.length < 3) {
        morningSlots.push("");
      }
      return morningSlots;
    }
    // 下午时段返回后4个元素
    else if (period === "afternoon") {
      const afternoonSlots = slots.value[slotIndex].slice(
        choiceMorningSlots,
        7
      );
      // 确保返回的数组始终有4个元素
      while (afternoonSlots.length < 4) {
        afternoonSlots.push("");
      }
      return afternoonSlots;
    }
  }
  // 根据时段返回默认数组
  if (period === "morning") {
    return ["", "", ""];
  } else {
    return ["", "", "", ""];
  }
}

function judgehDate(dateTab: string) {
  switch (dateTab) {
    case "date1":
      return 0;
    case "date2":
      return 1;
    case "date3":
      return 2;
    case "date4":
      return 3;
    case "date5":
      return 4;
    case "date6":
      return 5;
    case "date7":
      return 6;
  }
  return 0;
}

function switchSlots(dateTab: string, choiceActive: string) {
  const slotIndex = judgehDate(dateTab); //判断选择的时间段
  // 确保数组存在再访问
  if (slots.value && slots.value[slotIndex]) {
    switch (choiceActive) {
      case "button1":
        upSlot.value = slots.value[slotIndex][0] || "";
        break;
      case "button2":
        upSlot.value = slots.value[slotIndex][1] || "";
        break;
      case "button3":
        upSlot.value = slots.value[slotIndex][2] || "";
        break;
      case "button4":
        upSlot.value = slots.value[slotIndex][3] || "";
        break;
      case "button5":
        upSlot.value = slots.value[slotIndex][4] || "";
        break;
      case "button6":
        upSlot.value = slots.value[slotIndex][5] || "";
        break;
      case "button7":
        upSlot.value = slots.value[slotIndex][6] || "";
        break;
    }
  }
}
function submit(dateTab: string, choiceActive: string) {
  if (dateTab === "date1") {
    upYear.value = year.value[0];
    upMonth.value = month.value[0];
    upDay.value = day.value[0];
    switchSlots(dateTab, choiceActive);
  } else if (dateTab === "date2") {
    upYear.value = year.value[1];
    upMonth.value = month.value[1];
    upDay.value = day.value[1];
    switchSlots(dateTab, choiceActive);
  } else if (dateTab === "date3") {
    upYear.value = year.value[2];
    upMonth.value = month.value[2];
    upDay.value = day.value[2];
    switchSlots(dateTab, choiceActive);
  } else if (dateTab === "date4") {
    upYear.value = year.value[3];
    upMonth.value = month.value[3];
    upDay.value = day.value[3];
    switchSlots(dateTab, choiceActive);
  } else if (dateTab === "date5") {
    upYear.value = year.value[4];
    upMonth.value = month.value[4];
    upDay.value = day.value[4];
    switchSlots(dateTab, choiceActive);
  } else if (dateTab === "date6") {
    upYear.value = year.value[5];
    upMonth.value = month.value[5];
    upDay.value = day.value[5];
    switchSlots(dateTab, choiceActive);
  } else if (dateTab === "date7") {
    upYear.value = year.value[6];
    upMonth.value = month.value[6];
    upDay.value = day.value[6];
    switchSlots(dateTab, choiceActive);
  }
  // 提交预约记录
  store
    .dispatch("auth/upScheduleTime", {
      upDoctorId: upDoctorId.value,
      upYear: upYear.value,
      upMonth: upMonth.value,
      upDay: upDay.value,
      upSlot: upSlot.value,
    })
    .then((response) => {
      if (response.data.success && response.status === 200) {
        submitAfter.value = true;
        // 添加 emit 通知父组件预约成功
        emit("submit-success", {
          year: upYear.value,
          month: upMonth.value,
          day: upDay.value,
          slot: upSlot.value,
        });
        upDoctorId.value = 0;
      }
    })
    .catch((error) => {
      console.error("预约提交失败:", error);
    });
}
function switchDate(tab: string) {
  dateTab.value = tab;
  choiceActive.value = "";
}
function switchChoice(tab: string) {
  if (choiceActive.value !== tab) {
    choiceActive.value = tab;
  } else {
    choiceActive.value = "";
  }
}

// 移除提交成功后的状态
function removeSubmitAfter() {
  submitAfter.value = false;
  props.switchTab("reservation");
}

// 7. 生命周期钩子
onMounted(() => {
  // 可以在这里执行初始化操作
});

onBeforeUnmount(() => {
  // 可以在这里执行清理操作
});

// 8. 监听器 (如需要)
// import { watch, watchEffect } from "vue"
// watch(() => formData.name, (newVal, oldVal) => {
//   console.log("Name changed:", newVal)
// });

// 9. 路由相关 (如需要)
// const router = useRouter()
// const route = useRoute()
// const goToDetail = (id: number) => {
//   router.push(`/detail/id`)
// };
</script>

<style scoped lang="scss">
.reservation-slots {
  min-width: 52vw;
  display: flex;
  flex-direction: column;
  align-items: center;
  padding: 24px;
  margin-right: 10vw;
  margin-bottom: 100px;
  box-shadow: 0px 0px 5px 2px rgba(0, 0, 0, 0.3);
  border-radius: 12px;
  position: relative;

  .close-button {
    position: absolute;
    top: 16px;
    right: 16px;
    cursor: pointer;
    z-index: 10;
    color: #333;
  }

  .reservation-slots-page {
    width: 100%;
    height: 100%;
    display: flex;
    flex-direction: column;
    align-items: center;
    position: relative;

    .reservation-slots-top {
      width: 100%;
      height: 50px;
      border-bottom: 1px solid #e0e0e0;
      margin-bottom: 20px;

      .title-container {
        width: 100%;
        height: 100%;
        display: flex;
        justify-content: center;
        align-items: center;
        font-size: 30px;
        font-weight: 520;
      }
    }
    .reservation-slots-middle {
      width: 100%;
      height: 100%;
      display: flex;
      gap: 50px;
      flex-flow: row wrap;

      .reservation-slots-middle-div {
        display: flex;
        flex-direction: column;
        align-items: center;
        gap: 10px;
        .reservation-slots-middle-div-img {
          width: 200px;
          height: 250px;
          object-fit: cover;
          border-radius: 12px;
          box-shadow: 0px 0px 5px 2px rgba(0, 0, 0, 0.3);
        }
        .reservation-slots-middle-div-doctorName {
          display: flex;
          align-items: center;
          justify-content: center;
          flex-direction: column;
          font-size: 36px;
        }
        .reservation-slots-middle-div-span {
          width: 250px;
          height: 300px;
          font-size: 15px;
          font-weight: 520;
          display: flex;
          align-items: start;
          justify-content: left;
          flex-direction: row;
        }
        .reservation-slots-middle-div-button {
          width: 100px;
          height: 40px;
          border-radius: 12px;
          background-color: #42b983;
          font-size: 20px;
          font-weight: 520;
          padding: 0;
          display: flex;
          justify-content: center;
          align-items: center;
        }
        .reservation-slots-middle-div-button:active {
          transform: scale(0.96); /*点击时轻微缩小 */
          box-shadow: 3px 3px 10px rgba(0, 0, 0, 0.3); /*添加阴影效果 */
        }
      }
    }
    .reservation-slots-bottom {
      width: 50%;
      margin-top: 30px;
      .reservation-slots-bottom-container {
        width: 100%;
        display: flex;
        justify-content: space-between;

        .reservation-slots-bottom-container-cancle,
        .reservation-slots-bottom-container-submit {
          width: 30%;
          padding: 0;
          font-size: 25px;
          border-radius: 15px;
          display: flex;
          justify-content: center;
          align-items: center;
        }
        .reservation-slots-bottom-container-cancle {
          background-color: #f24444;
        }
        .reservation-slots-bottom-container-submit {
          background-color: #42b983;
        }
        .reservation-slots-bottom-container-cancle:active,
        .reservation-slots-bottom-container-submit:active {
          transform: scale(0.98); /*点击时轻微缩小 */
          box-shadow: 3px 3px 10px rgba(0, 0, 0, 0.3); /*添加阴影效果 */
        }
      }
    }
    .date-selectors {
      width: 100%;

      .date-selectors-top {
        position: relative;
        display: flex;
        flex-direction: row;
        gap: 50px;

        .date-selectors-top-button-container {
          position: relative;
          display: flex;
          flex-direction: row;
          gap: 25px;

          .date-selectors-top-button-div {
            .date-selectors-top-button1,
            .date-selectors-top-button2,
            .date-selectors-top-button3,
            .date-selectors-top-button4,
            .date-selectors-top-button5,
            .date-selectors-top-button6,
            .date-selectors-top-button7 {
              box-shadow: 0px 0px 5px 2px rgba(0, 0, 0, 0.3);
              display: flex;
              flex-direction: column;
              align-items: start;
              border-radius: 6px;
              padding: 3px;
            }
            .date-selectors-top-button1:active,
            .date-selectors-top-button2:active,
            .date-selectors-top-button3:active,
            .date-selectors-top-button4:active,
            .date-selectors-top-button5:active,
            .date-selectors-top-button6:active,
            .date-selectors-top-button7:active {
              transform: scale(0.96); /*点击时轻微缩小 */
              box-shadow: 3px 3px 10px rgba(0, 0, 0, 0.3); /*添加阴影效果 */
            }
            .date-selectors-top-button1.click,
            .date-selectors-top-button2.click,
            .date-selectors-top-button3.click,
            .date-selectors-top-button4.click,
            .date-selectors-top-button5.click,
            .date-selectors-top-button6.click,
            .date-selectors-top-button7.click {
              box-shadow: 0px 0px 5px 2px rgba(0, 0, 0, 0.3);
              display: flex;
              flex-direction: column;
              align-items: start;
              border-radius: 6px;
              padding: 3px;
              background-color: #42b983;
              transition: all 1.5s ease;
            }
          }
        }
      }
      .date-selectors-middle {
        width: 100%;

        .date-selectors-middle-slots {
          width: 100%;
          display: flex;
          align-items: center;
          flex-direction: column;
          justify-content: space-between;
        }
      }
    }
  }
}
.date-selectors-top-button-text {
  font-size: 13px;
  font-weight: 520;
}
span {
  font-size: 25px;
  font-weight: 520;
}
.date-selectors-middle-slots-title {
  font-size: 30px;
  font-weight: 520;
  display: flex;
  align-items: start;
  margin-top: 30px;
}
.date-selectors-middle-slots-morning,
.date-selectors-middle-slots-afternoon {
  width: 100%;
  position: relative;
  display: flex;
  justify-content: space-between;
  .date-selectors-middle-slots-morning-container,
  .date-selectors-middle-slots-afternoon-container {
    width: 61%;
    display: flex;
    justify-content: space-between;
    gap: 20px;
    margin-top: 30px;
    .date-selectors-middle-slots-morning-container-left,
    .date-selectors-middle-slots-afternoon-container-left {
      display: flex;
      flex-direction: column;
    }
    .date-selectors-middle-slots-morning-container-right,
    .date-selectors-middle-slots-afternoon-container-right {
      display: flex;
      flex-direction: column;

      .date-selectors-middle-slots-morning-container-right-button {
        background: none;
        cursor: pointer;
        display: flex;
        position: relative;
        width: 14px;
        font: 12px/1.5 PingFangSC-Regular, Tahoma, Microsoft Yahei, sans-serif;
        margin: 0;
        padding: 0;
        border: none;
        outline: inherit;

        .date-selectors-middle-slots-morning-container-right-img {
          width: 28px;
          height: 28px;
          margin-bottom: 12px;
        }
      }
    }
  }
}
.date-selectors-middle-slots-morning {
  margin-bottom: 20px;
}

.reservation-slots-bottom-container-submit-after {
  height: 300px;
  width: 420px;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  gap: 30px;
  position: fixed;
  top: 35%;
  left: 50%;
  transform: translateX(-50%);
  background: rgb(255, 255, 255);
  color: black;
  padding: 20px;
  border-radius: 12px;
  box-shadow: 0 4px 8px rgba(0, 0, 0, 0.4);
  z-index: 1000;
  text-align: center;
  font-size: 25px;

  .reservation-slots-bottom-container-submit-after-button {
    width: 80px;
    height: 45px;
    padding: 15px;
    background: #4caf50;
    font-size: 25px;
    font-weight: 520;
    border-radius: 12px;
    display: flex;
    justify-content: center;
    align-items: center;
  }
}
</style>
