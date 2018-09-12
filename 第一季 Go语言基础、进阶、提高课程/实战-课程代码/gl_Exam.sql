/*
Navicat MySQL Data Transfer

Source Server         : 问答社区
Source Server Version : 50173
Source Host           : 120.24.219.60:3306
Source Database       : gl_Exam

Target Server Type    : MYSQL
Target Server Version : 50173
File Encoding         : 65001

Date: 2018-09-12 20:00:56
*/

SET FOREIGN_KEY_CHECKS=0;

-- ----------------------------
-- Table structure for `t_ExamData`
-- ----------------------------
DROP TABLE IF EXISTS `t_ExamData`;
CREATE TABLE `t_ExamData` (
  `ID` int(11) NOT NULL AUTO_INCREMENT COMMENT '试题的ID',
  `Name` varchar(50) NOT NULL COMMENT '试卷的名字或者标题',
  `Type` varchar(11) NOT NULL COMMENT '试卷的类型：java golang c',
  `Price` varchar(11) NOT NULL COMMENT '金币或者货币',
  `Msg` varchar(300) NOT NULL COMMENT '试卷的描述信息',
  `Createtime` varchar(20) NOT NULL COMMENT '试卷的创建时间',
  `Time` varchar(20) NOT NULL COMMENT '预发售时间',
  PRIMARY KEY (`ID`)
) ENGINE=MyISAM AUTO_INCREMENT=10 DEFAULT CHARSET=utf8;

-- ----------------------------
-- Records of t_ExamData
-- ----------------------------
INSERT INTO `t_ExamData` VALUES ('1', '彬哥-Go语言基础测试题', 'golang', '10', '基础知识过关必过开始', '1536207377', '');
INSERT INTO `t_ExamData` VALUES ('2', '彬哥-Go语言进阶测试题', 'golang', '5', '进阶知识过关必过开始', '1536207377', '');
INSERT INTO `t_ExamData` VALUES ('3', '彬哥-Go语言进阶测试题1', 'golang', '2', '基础知识过关必过开始', '1536207377', '');
INSERT INTO `t_ExamData` VALUES ('4', '彬哥-Go语言进阶测试题2', 'golang', '2', '基础知识过关必过开始', '1536207377', '');
INSERT INTO `t_ExamData` VALUES ('5', '彬哥-Go语言进阶测试题3', 'golang', '2', '基础知识过关必过开始', '1536207377', '');
INSERT INTO `t_ExamData` VALUES ('6', '彬哥-Go语言进阶测试题4', 'golang', '2', '基础知识过关必过开始', '1536207377', '');
INSERT INTO `t_ExamData` VALUES ('7', '彬哥-Go语言进阶测试题5', 'java', '3', '基础知识过关必过开始', '1536207377', '1536380177');
INSERT INTO `t_ExamData` VALUES ('8', '彬哥-Go语言进阶测试题5', 'java', '14', '', '1536207377', '1536380177');
INSERT INTO `t_ExamData` VALUES ('9', '彬哥1234567', 'c  ', '100', 'ddddddaada', '1536310821', '1527687888');

-- ----------------------------
-- Table structure for `t_UserExam`
-- ----------------------------
DROP TABLE IF EXISTS `t_UserExam`;
CREATE TABLE `t_UserExam` (
  `ID` int(11) NOT NULL AUTO_INCREMENT,
  `OpenID` varchar(70) NOT NULL COMMENT '用户的唯一的ID',
  `Exam_yj` varchar(500) NOT NULL COMMENT '已经参加的开始',
  `Exam_jy` varchar(100) NOT NULL COMMENT '预报名考试',
  `Createtime` varchar(50) NOT NULL,
  PRIMARY KEY (`ID`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- ----------------------------
-- Records of t_UserExam
-- ----------------------------

-- ----------------------------
-- Table structure for `t_UserInfo`
-- ----------------------------
DROP TABLE IF EXISTS `t_UserInfo`;
CREATE TABLE `t_UserInfo` (
  `ID` int(11) NOT NULL AUTO_INCREMENT,
  `Name` varchar(100) NOT NULL,
  `Sex` int(11) NOT NULL,
  `OpenID` varchar(70) NOT NULL,
  `HeadUrl` varchar(200) NOT NULL,
  `Createtime` varchar(20) NOT NULL,
  PRIMARY KEY (`ID`)
) ENGINE=MyISAM AUTO_INCREMENT=2 DEFAULT CHARSET=utf8;

-- ----------------------------
-- Records of t_UserInfo
-- ----------------------------
INSERT INTO `t_UserInfo` VALUES ('1', '李海彬', '1', 'ahajdadhaduasdnad', '', '1536207377');
