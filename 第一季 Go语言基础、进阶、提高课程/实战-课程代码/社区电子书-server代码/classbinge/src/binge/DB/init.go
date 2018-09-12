package DB

import (
	"database/sql"
	"glog-master"

	_ "github.com/go-sql-driver/mysql"
)

// 链接池的最大链接数量
const MAX_POOL_SIZE int = 200

// 全局数据库变量
var MySQLPool chan *sql.DB

// 获取数据链接
func getMySQL() *sql.DB {
	// 获取链接
	conn := GetMySQL1()
	// 压入队列
	putMySQL(conn)
	return conn
}

// 获取数据链接
func GetMySQL() *sql.DB {
	// 获取链接
	conn := GetMySQL1()
	// 压入队列
	putMySQL(conn)
	return conn
}

// 获取链接指针函数
func GetMySQL1() *sql.DB {
	if MySQLPool == nil {
		MySQLPool = make(chan *sql.DB, MAX_POOL_SIZE)
	}
	if len(MySQLPool) == 0 {
		go func() {
			for i := 0; i < MAX_POOL_SIZE/2; i++ {
				mysql := new(sql.DB)
				var err error
				var StrConnection = ""
				StrConnection = "root" + ":" + "密码" + "@tcp(" + "IP" + ":3306)/" + "gl_Exam"
				mysql, err = sql.Open("mysql", StrConnection)
				if err != nil {
					glog.Info("Connect Fail!")
					continue
				}
				putMySQL(mysql)
			}
		}()
	}
	return <-MySQLPool
}

//存储指针函数
func putMySQL(conn *sql.DB) {
	if MySQLPool == nil {
		MySQLPool = make(chan *sql.DB, MAX_POOL_SIZE)
	}
	if len(MySQLPool) == MAX_POOL_SIZE {
		conn.Close()
		return
	}
	MySQLPool <- conn
}
