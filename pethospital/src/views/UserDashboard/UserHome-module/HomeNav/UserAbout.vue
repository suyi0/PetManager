<template>
  <div class="about-view">
    <div class="about-container">
      <div class="about-container-middle">
        <div class="about-container-left">
          <div
            class="about-left1"
            :class="{ ' click ': activeTab === 'environment' }"
            @click="switchTab('environment')"
          >
            <button class="about-left1-button">
              <span> 医院环境 </span>
            </button>
          </div>
          <div
            class="about-left2"
            :class="{ ' click ': activeTab === 'doctor' }"
            @click="switchTab('doctor')"
          >
            <button class="about-left2-button"><span>医生团队</span></button>
          </div>
          <div
            class="about-left3"
            :class="{ ' click ': activeTab === 'machine' }"
            @click="switchTab('machine')"
          >
            <button class="about-left3-button"><span>设备优势</span></button>
          </div>
          <div
            class="about-left4"
            :class="{ ' click ': activeTab === 'advantage' }"
            @click="switchTab('advantage')"
          >
            <button class="about-left4-button"><span>服务优势</span></button>
          </div>
          <div
            class="about-left5"
            :class="{ ' click ': activeTab === 'consultingService' }"
            @click="switchTab('consultingService')"
          >
            <button class="about-left5-button"><span>免费咨询</span></button>
          </div>
          <div
            class="about-left6"
            :class="{ ' click ': activeTab === 'contact' }"
            @click="switchTab('contact')"
          >
            <button class="about-left6-button"><span>联系我们</span></button>
          </div>
        </div>
        <div class="about-container-right">
          <div
            v-if="activeTab === 'environment'"
            class="about-container-right-environment"
            :style="{ backgroundImage: `url(${backgroundImage})` }"
          >
            <div></div>
          </div>
          <div
            v-if="activeTab === 'doctor'"
            class="about-container-right-doctor"
          ></div>
          <div
            v-if="activeTab === 'machine'"
            class="about-container-right-machine"
          ></div>
          <div
            v-if="activeTab === 'advantage'"
            class="about-container-right-advantage"
          ></div>
          <div
            v-if="activeTab === 'consultingService'"
            class="about-container-right-consultingService"
          ></div>
          <div
            v-if="activeTab === 'contact'"
            class="about-container-right-contact"
          >
            <div class="contact-container">
              <h2 class="contact-container-h2">联系方式</h2>
              <span class="contact-container-span1"
                >(如果有什么问题请通过以下方式联系)</span
              >
              <span class="contact-container-span2">电话: 123-456-7890</span>
              <span class="contact-container-span3">邮箱: *************</span>
              <div class="contact-container-WeChat">
                <img src="" alt="QR" />
                <span class="contact-container-span4">微信公众号</span>
              </div>
            </div>
          </div>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { defineComponent, ref } from "vue";
import backgroundImage from "@/assets/photo/background.jpeg";

defineComponent({
  name: "userHomeNavAbout",
});

const activeTab = ref("environment");

const switchTab = (tab: string) => {
  activeTab.value = tab;
};
</script>

<style lang="css">
.about-view {
  width: 100%;
  height: 100%;
  display: flex;
  flex-direction: row;
  align-items: center;
}
button {
  font-size: 20px;
  font-weight: 520;
  width: 100%;
  height: 100%;
}

.about-container {
  width: 100%;
  height: 100%;
  background-color: rgb(255, 255, 255);
}

.about-container-middle {
  display: flex;
  flex-direction: row;
}

.about-container-left {
  min-width: 19vw;
  font-size: 18px;
  font-weight: 520;
  display: flex;
  flex-direction: column;
  text-align: center;
  margin-top: 104px;
  margin-right: 85px;
  position: sticky;
  z-index: 1001;
  .about-left1 {
    border-top: 2px solid rgb(0, 0, 0);
  }
  .about-left1,
  .about-left2,
  .about-left3,
  .about-left4,
  .about-left5,
  .about-left6 {
    width: 100%;
    height: 68px;
    padding-bottom: 7px;
    padding-top: 7px;
    border-bottom: 2px solid rgb(0, 0, 0);
    border-right: 2px solid rgb(0, 0, 0);
    border-radius: 8px;
  }
  .about-left1:active,
  .about-left2:active,
  .about-left3:active,
  .about-left4:active,
  .about-left5:active,
  .about-left6:active {
    transform: scale(0.98); /*点击时轻微缩小 */
    box-shadow: 3px 3px 10px rgba(0, 0, 0, 0.3);
  }
  .about-left1.click,
  .about-left2.click,
  .about-left3.click,
  .about-left4.click,
  .about-left5.click,
  .about-left6.click {
    background-color: rgba(80, 160, 247, 0.3);
    color: white;
    span {
      color: #42b983;
    }
  }
  .about-left1-button,
  .about-left2-button,
  .about-left3-button,
  .about-left4-button,
  .about-left5-button,
  .about-left6-button {
    width: 100%;
    height: 100%;
    text-align: center;
  }
}

.about-container-right {
  position: absolute;
  display: flex;
  width: 100%;
  height: 100%;
  border-radius: 20px;
  align-items: center;
  flex-direction: column;
  z-index: 1;
  .about-container-right-environment {
    width: 100%;
    height: 100%;
    position: absolute;
    background-size: cover;
    background-position: center;
  }
  .about-container-right-doctor {
    position: absolute;
    left: 25%;
  }
  .about-container-right-machine {
    position: absolute;
    left: 25%;
  }
  .about-container-right-advantage {
    position: absolute;
    left: 25%;
  }
  .about-container-right-consultingService {
    position: absolute;
    left: 25%;
  }
  .about-container-right-contact {
    position: absolute;
    top: 124px;
    left: 35%;
    width: 45vw;
    padding: 20px;
    border-radius: 20px;
    box-shadow: 0px 0px 5px 2px rgba(0, 0, 0, 0.3);
    .contact-container {
      display: flex;
      flex-direction: column;
      align-items: center;
      gap: 20px;
      font-size: 20px;
      font-weight: 400;
    }
    .contact-container-h2 {
      font-size: 30px;
      font-weight: 520;
      margin-top: 10px;
      margin-bottom: 20px;
    }
    .contact-container-span1 {
      color: rgba(255, 0, 0, 0.6);
    }
    .contact-container-span2,
    .contact-container-span3 {
      margin-bottom: 10px;
    }
    .contact-container-WeChat {
      display: flex;
      flex-direction: column;
      align-items: center;
      img {
        width: 150px;
        height: 150px;
        margin-bottom: 10px;
      }
    }
  }
}
</style>
