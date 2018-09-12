App({
  /**
   * 当小程序初始化完成时，会触发 onLaunch（全局只触发一次）
   */
  onLaunch: function () {
    // var that = this;
    // wx.login({
    //   success: res => {
    //     wx.request({
    //       url: that.globalData.wx_url_1 + res.code + that.globalData.wx_url_2,
    //       success: res => {
    //         that.globalData.openid = res.data.openid;
    //       }
    //     })
    //   }
    // });
    console.log("ss")
    wx.navigateTo({
      url: './pages/book/book'　　// 跳转页面
    })
  },

  /**
   * 设置全局变量
   */
  globalData: {
    openid: 0,
    wx_url_1: 'https://api.weixin.qq.com/sns/jscode2session?appid=wx97bf8844d999ad95&secret=728dc7c591f7e087cab7ed60911f6ccc&js_code=',
    wx_url_2: '&grant_type=authorization_code'
  }
})
