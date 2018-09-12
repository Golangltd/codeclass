
var app = getApp()
Page({
  data: {
    userInfo: {},
    userListInfo: [{
      icon: '../../dist/images/iconfont-dingdan.png',
      text: '我的订单',
      isunread: true,
      unreadNum: 2
    }, {
      icon: '../../dist/images/iconfont-card.png',
      text: '我的代金券',
      isunread: true,
      unreadNum: 5
    }, {
      icon: '../../dist/images/iconfont-icontuan.png',
      text: '我参与的考试',
      isunread: true,
      unreadNum: 1
    },{
      icon: '../../dist/images/iconfont-help.png',
      text: '常见问题'
    }]
  },
  onLoad: function () {
    var that = this
    //调用应用实例的方法获取全局数据
    app.getUserInfo(function (userInfo) {
      //更新数据
      that.setData({
        userInfo: userInfo
      })
    })
  }
})