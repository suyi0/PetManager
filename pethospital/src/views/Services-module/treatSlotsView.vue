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
        <div class="reservation-slots-middle-div">
          <img
            class="reservation-slots-middle-div-img"
            src="@/assets/photo/background.jpeg"
            alt="头像"
          />
          <span class="reservation-slots-middle-div-span">医生介绍:</span>
          <button
            class="reservation-slots-middle-div-button"
            @click="props.switchTab('showSlots')"
          >
            <span>预约</span>
          </button>
        </div>
        <div class="reservation-slots-middle-div">
          <img
            class="reservation-slots-middle-div-img"
            src="@/assets/photo/background.jpeg"
            alt="头像"
          />
          <span class="reservation-slots-middle-div-span">医生介绍:</span>
          <button
            class="reservation-slots-middle-div-button"
            @click="props.switchTab('showSlots')"
          >
            <span>预约</span>
          </button>
        </div>
        <div class="reservation-slots-middle-div">
          <img
            class="reservation-slots-middle-div-img"
            src="@/assets/photo/background.jpeg"
            alt="头像"
          />
          <span class="reservation-slots-middle-div-span">医生介绍:</span>
          <button
            class="reservation-slots-middle-div-button"
            @click="props.switchTab('showSlots')"
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
            <span class="date-selectors-top-button-text"
              >{{ props.year[0] }}年
            </span>
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
                    {{ props.week[0] }}
                  </span>
                  <span class="date-selectors-top-button-text"
                    >{{ props.month[0] }}月{{ props.day[0] }}日</span
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
                    {{ props.week[1] }}
                  </span>
                  <span class="date-selectors-top-button-text"
                    >{{ props.month[1] }}月{{ props.day[1] }}日</span
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
                    {{ props.week[2] }}
                  </span>
                  <span class="date-selectors-top-button-text"
                    >{{ props.month[2] }}月{{ props.day[2] }}日</span
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
                    {{ props.week[3] }}
                  </span>
                  <span class="date-selectors-top-button-text"
                    >{{ props.month[3] }}月{{ props.day[3] }}日</span
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
                    {{ props.week[4] }}
                  </span>
                  <span class="date-selectors-top-button-text"
                    >{{ props.month[4] }}月{{ props.day[4] }}日</span
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
                    {{ props.week[5] }}
                  </span>
                  <span class="date-selectors-top-button-text"
                    >{{ props.month[5] }}月{{ props.day[5] }}日</span
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
                    {{ props.week[6] }}
                  </span>
                  <span class="date-selectors-top-button-text"
                    >{{ props.month[6] }}月{{ props.day[6] }}日</span
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
                    <span>{{ props.slots[0][0] }}</span>
                    <span>{{ props.slots[0][1] }}</span>
                    <span>{{ props.slots[0][2] }}</span>
                  </div>
                  <div
                    class="date-selectors-middle-slots-morning-container-right"
                  >
                    <button
                      class="date-selectors-middle-slots-morning-container-right-button"
                      @click="switchChoice('button1')"
                    >
                      <img
                        v-if="choiceActive === 'button1'"
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
                    <button
                      class="date-selectors-middle-slots-morning-container-right-button"
                      @click="switchChoice('button2')"
                    >
                      <img
                        v-if="choiceActive === 'button2'"
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
                    <button
                      class="date-selectors-middle-slots-morning-container-right-button"
                      @click="switchChoice('button3')"
                    >
                      <img
                        v-if="choiceActive === 'button3'"
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
                </div>
              </div>
              <div class="date-selectors-middle-slots-afternoon">
                <span class="date-selectors-middle-slots-title">下午</span>
                <div class="date-selectors-middle-slots-afternoon-container">
                  <div
                    class="date-selectors-middle-slots-afternoon-container-left"
                  >
                    <span>{{ props.slots[0][3] }}</span>
                    <span>{{ props.slots[0][4] }}</span>
                    <span>{{ props.slots[0][5] }}</span>
                    <span>{{ props.slots[0][6] }}</span>
                  </div>
                  <div
                    class="date-selectors-middle-slots-afternoon-container-right"
                  >
                    <button
                      class="date-selectors-middle-slots-morning-container-right-button"
                      @click="switchChoice('button4')"
                    >
                      <img
                        v-if="choiceActive === 'button4'"
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
                    <button
                      class="date-selectors-middle-slots-morning-container-right-button"
                      @click="switchChoice('button5')"
                    >
                      <img
                        v-if="choiceActive === 'button5'"
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
                    <button
                      class="date-selectors-middle-slots-morning-container-right-button"
                      @click="switchChoice('button6')"
                    >
                      <img
                        v-if="choiceActive === 'button6'"
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
                    <button
                      class="date-selectors-middle-slots-morning-container-right-button"
                      @click="switchChoice('button7')"
                    >
                      <img
                        v-if="choiceActive === 'button7'"
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
            @click="submit"
          >
            确定
          </button>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
// 1. 导入部分
import { ref, onMounted, onBeforeUnmount } from "vue";
import { useStore } from "vuex";
import { key } from "@/store";
// import { useRouter, useRoute } from "vue-router";
// import type { PropType } from "vue";

const store = useStore(key);
store;

// 2. Props定义 (如果需要接收父组件传递的数据)
const props = defineProps<{
  activeTab: string;
  switchTab(_tab: string): void;
  year: string[];
  month: string[];
  day: string[];
  week: string[];
  slots: string[][];
}>();

// 3. Emit事件定义 (如果需要向父组件传递事件)
const emit = defineEmits(["close", "submit", "cancle"]);

// 4. 响应式数据
const dateTab = ref("date1");
const choiceActive = ref("");

// 5. 计算属性

// 6. 方法定义
function close() {
  emit("close");
}
function cancle() {
  emit("cancle");
}
function submit() {
  emit("submit");
}
function switchDate(tab: string) {
  dateTab.value = tab;
}
function switchChoice(tab: string) {
  choiceActive.value = tab;
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
</style>
