package match

type MatchMoudle interface {
	GetMatchResult(string, int) []byte
	PutMatch([]byte)
	GetMatchNum(string) int
}
