package ST_Data

// 考试的结构数据信息
// list {id,name,... ... }
type ExamST struct {
	ID         int    // 试卷的ID信息
	Name       string // 试卷的名字
	Type       string // 试卷类型 JAVA GO C++ ... ...
	Price      string // 试卷价格
	Msg        string // 试卷的描述信息
	CreateTime string // 创建的时间
	Time       string // 创建的时间
	State      string // 1：已经参与的考试 2：其他的，不关心
}
