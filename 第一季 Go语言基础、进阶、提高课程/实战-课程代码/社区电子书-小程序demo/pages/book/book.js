
//book.js
//获取应用实例
const app = getApp()

Page({

  /**
   * 页面的初始数据
   */
  data: {
    //轮播图配置
    autoplay: true,
    interval: 3000,
    duration: 1200,
    bookList:[],
    pageNum: 0,       // 设置加载的第几次，默认是第一次
    isFirstLoad: true,   // 用于判断List数组是不是空数组，默认true，空的数组
    hasMore: false,    // “加载更多”
  },

  /**
   * 生命周期函数--监听页面加载
   */
  onLoad: function (options) {
    console.log("func")
    var that = this;
    var data = {
      "datas": [
        {
          "id": 1,
          "imgurl": "../../images/lunbo/a1.jpg"
        },
        {
          "id": 2,
          "imgurl": "../../images/lunbo/a2.jpg"
        },
        {
          "id": 3,
          "imgurl": "../../images/lunbo/a3.jpg"
        },
        {
          "id": 4,
          "imgurl": "../../images/lunbo/a4.jpg"
        }
      ]
    };
    // 设置数据
    that.setData({
      lunboData: data.datas,
    });
    wx.request({
      url: "http://local.bytedancing.com:8080?Proto=2&pagenum=0",
      success: function (res) {
        console.log(res.data)
        // console.log(res.data[5].Msg)
        // 设置数据
        that.setData({
          bookList: res.data
        })
      },
      fail: function (err) {
        console.log(err)
      }
    })
  },
  // 下拉刷新
  onPullDownRefresh: function () {
    let that = this;
    console.log('--------下拉刷新-------')
    // 显示导航栏loading
    wx.showNavigationBarLoading();
    // 调用接口加载数据
   //this.loadData();
    wx.request({
      url: "http://local.bytedancing.com:8080?Proto=2&pagenum=0",
      success: function (res) {
        console.log(res.data)
        // console.log(res.data[5].Msg)
        var datatmp = res.data
        var dataarr = []
        var icount = 0
        for (var i in datatmp) {
          dataarr.push(datatmp[i])
          console.log(icount)
          if (icount == 0) {
            icount = datatmp[i]["ID"]
          } else {
            // 获取ID最大的数据
            if (icount > datatmp[i]["ID"]) {
              icount = datatmp[i]["ID"]
            }
          }
        }
        // 设置数据
        that.setData({
          pageNum: icount - 1,
          bookList: res.data
        })
      },
      fail: function (err) {
        console.log(err)
      },
      complete: function () {
        // 隐藏导航栏loading
        wx.hideNavigationBarLoading();
        // 当处理完数据刷新后，wx.stopPullDownRefresh可以停止当前页面的下拉刷新
        wx.stopPullDownRefresh();
      }
    })
  },
  // 上拉加载
  /**
   * 页面上拉触底事件的处理函数
   */
  onReachBottom: function () {
    var that = this;
    // 显示加载图标
    wx.showLoading({
      title: '玩命加载中',
    })
    console.log(that.data.pageNum)
    wx.request({
      url: 'http://local.bytedancing.com:8080?Proto=2&pagenum=' + that.data.pageNum,
      success: function (res) {
        console.log(res.data)
        var datatmp = res.data
        var dataarr = []
        var icount = 0
        for (var i in datatmp){
          dataarr.push(datatmp[i])
          console.log(icount)
          if (icount == 0){
            icount = datatmp[i]["ID"]
          }else{
            // 获取ID最大的数据
            if (icount > datatmp[i]["ID"]){
               icount = datatmp[i]["ID"]
            }
          }
        }
        console.log(dataarr)
        // 回调函数
        // 设置数据
        that.setData({
          pageNum: icount + 1,
          isFirstLoad: false ,
          bookList: dataarr
        })
        // 隐藏加载框
        wx.hideLoading();
      }
    })
  },


})