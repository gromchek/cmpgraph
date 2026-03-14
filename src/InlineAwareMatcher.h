#ifndef INLINE_AWARE_MATCHER_H
#define INLINE_AWARE_MATCHER_H

#include "FunctionMatcher.h"

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <thread>

// ---------------------------------------------------------------------------
// InlineAwareMatcher
//
// Дополнительный слой поверх FunctionMatcher, который обрабатывает случай,
// когда одна из версий программы содержит инлайнинг функций.
//
// Идея:
//   Если B заинлайнена в A, то в «целевом» графе (g2) ребро A->C существует,
//   а в «эталонном» (g1) — только путь A->B->C.
//
//   Решение — строить множества достижимости (reachability sets) за k шагов
//   и сравнивать их вместо непосредственных соседей.
//
// Использование:
//   1. Сначала запустите FunctionMatcher::runFullMatch() — он заполнит match.
//   2. Передайте результаты match в InlineAwareMatcher::run().
//   3. Класс вернёт дополнительные пары (g2_idx -> g1_idx) для функций,
//      которые не были сопоставлены на первом этапе.
// ---------------------------------------------------------------------------
class InlineAwareMatcher
{
public:
	// maxDepth — максимальная глубина обхода (2 или 3 рекомендуется).
	// scoreThreshold — минимальный score для принятия пары.
	InlineAwareMatcher( const std::vector<Function> &g1, const std::vector<Function> &g2, int maxDepth = 2,
						double scoreThreshold = 0.25 );

	// Запустить сопоставление.
	// existingMatch — уже найденные пары из FunctionMatcher (g2_idx -> g1_idx).
	// Возвращает новые пары, не пересекающиеся с existingMatch.
	std::unordered_map<int, int> run( const std::unordered_map<int, int> &existingMatch );

	// Вычислить score для конкретной пары (g2_idx, g1_idx).
	// Полезно для отладки или ручной проверки.
	double computeScore( int g2Idx, int g1Idx, const std::unordered_map<int, int> &match ) const;

private:
	const std::vector<Function> &g1_;
	const std::vector<Function> &g2_;
	int maxDepth_;
	double scoreThreshold_;

	// Индексы для быстрого перехода по именам
	std::unordered_map<std::string, int> nameToIdxG1_;
	std::unordered_map<std::string, std::vector<int>> nameToIndicesG2_;

	// -----------------------------------------------------------------------
	// Reachability sets
	// -----------------------------------------------------------------------

	// Для узла idx в графе g1 — множество имён функций, достижимых
	// по callees за [1..maxDepth_] шагов.
	std::vector<std::unordered_set<std::string>> reachCalleeG1_;
	// То же по callers.
	std::vector<std::unordered_set<std::string>> reachCallerG1_;

	// Аналогично для g2, но храним имена «эталонных» функций,
	// т.е. имена из g1, полученные через match.
	// Строится каждый раз при вызове run(), т.к. зависит от match.

	// -----------------------------------------------------------------------
	// Helpers
	// -----------------------------------------------------------------------

	void buildG1ReachSets();

	// Вернуть множество имён, достижимых из узла v графа g1 за [1..depth] шагов
	// по callees или callers (зависит от useCallees).
	std::unordered_set<std::string> bfsNames_G1( int start, bool useCallees, int depth ) const;

	// Вернуть множество имён g1, достижимых из узла v графа g2 за [1..depth]
	// шагов. Имена берём через match (пропускаем непереведённые узлы).
	std::unordered_set<std::string> bfsTranslated_G2( int start, bool useCallees, int depth,
													  const std::unordered_map<int, int> &match ) const;

	// Jaccard-подобный score между двумя множествами имён.
	static double overlapScore( const std::unordered_set<std::string> &a, const std::unordered_set<std::string> &b );

	bool isUnknown( const std::string &name ) const;
};

#endif // INLINE_AWARE_MATCHER_H