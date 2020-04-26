package internal

import (
	"sync"
)

//  房间结构
type SlotsRoom struct {
	RoomId       string
	SlotsType    int32 // 线数
	PlayerNum    int
	MaxPlayerNum int
	RoomPlayers  map[int32]*SlotsPlayer
}

// 房间指针管理器
type SlotsRoomST struct {
	SlotRoomData map[string]*SlotsRoom
	SlotsLock    *sync.RWMutex
}

// slot 玩家
type SlotsPlayer struct {
	ID          int32
	SlotsTypeId int32 //30000 - 31000 30线  90000 - 91000 1线
	//Slots
	LineNum          int64
	TotalSpin        int32
	TotalLine        int64
	TotalChips       int64
	TotalWin         int32
	TotalWinLine     int32
	CurTotalWinChips int64

	FanhuanLV     float32
	ZhongjiangLV  float32
	FreeSpin      int32
	FreeSpinLV    float32
	LeftFreeSpin  int32
	TotalFreeSpin int32
	Wild          int32
	WildLV        float32

	LineChip int64 //单条线的下注额//
	BaseChip int64 //基础下注额，永远是LineChip*LineNum//
	ChipIn   int64 //实际下注额，如果是freespin，那么chipin就是0//

	TouchJackPot1 int32 //中大奖的次数
	TouchJackPot2 int32 //中大奖的次数
	TouchJackPot3 int32 //中大奖的次数
	JackPotIn     int64
	JackPotOut    int64
	SlotsReward   bool

	Entry     *SpinEntry
	EntryPing int64

	Ping int64 //ping的时间戳

	Room   *SlotsRoom //房间//
	Player *model.Player
	//SpinNodes         map[int32]*SpinNode //玩家的spin节点//
	SpinNodes         *sync.Map //玩家的spin节点//
	PointControlValue int64
	currentWinChips   int64
	currentLoseChips  int64
	totalWinChips     int64
	totalLoseChips    int64

	SlotPickStar   bool
	PickStarCount  int32 //猜大小次数
	PickColorCount int32 //猜花色次数
	PickWinChips   int64

	//单线
	oneBetChips int64 //单线下注
	oneWinChips int64 //单线一回合赢取
	inOneGame   bool  //是否处于单线回合中

	MegaNext  int64
	MissCount int32
	//全线
	AllLineTriple *SlotAllLineTriple //全线管理器
}

// 所有线的管理器
type SlotAllLineTriple struct {
	roomType     string
	mConfigLevel int
	mWildCount   [SLOTALTMAXCOL]int
	mLastRet     *SlotALTTableResult
	mGameState   SlotALTGameState
	HitCnt       int //连续消除次数
	MissCnt      int //miss次数
	log          AllLineLog
}

// 房间管理器
var GSlotRoomPtr *SlotsRoomST

//------------------------------------------------------------------------------
// 获取指针管理器
func NewGSlotRoomPtr() *SlotsRoomST {
	return &SlotsRoomST{
		SlotRoomData: make(map[string]*SlotsRoom),
		SlotsLock:    new(sync.RWMutex),
	}
}

// -----------------------------------------------------------------------------

const (
	GEZI          = iota // 格子对应的数据
	SONGJIANG            // 宋江 （2D贴图，完整的人物3D动画）
	LINCHONG             // 林冲（2D贴图，完整的人物3D动画）
	LUZHISHEN            // 鲁智深（2D贴图，完整的人物3D动画）
	TITIANXINGDAO        // 替天行道（2D贴图，完整的3D动画）
	ZHONGYITANG          // 忠义堂（2D贴图，完整的3D动画）
	DADAO                // 大刀（2D贴图，完整的3D动画）
	CHNAGQIANG           // 长枪（2D贴图，完整的3D动画）
	SHUANGFU             // 双斧（2D贴图，完整的3D动画）
	SHUIHUZHUAN          // 水浒传（2D贴图，完整的3D动画）-万能图像可代替以上任意图像
	CAIJIN               // 彩金标记，使用小旗子标示（2D贴图）
)

// -----------------------------------------------------------------------------
