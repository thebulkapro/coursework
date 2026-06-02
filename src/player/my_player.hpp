#pragma once

#include "core/game.hpp"
#include <vector>

namespace ttt::my_player {

using game::Event;
using game::IPlayer;
using game::Point;
using game::Sign;
using game::State;

class HeuristicEvaluator {
public:
    static constexpr int SCORE_WIN = 1000000;
    static constexpr int SCORE_OPEN_4 = 50000;
    static constexpr int SCORE_CLOSED_4 = 10000;
    static constexpr int SCORE_OPEN_3 = 8000;
    static constexpr int SCORE_CLOSED_3 = 1000;
    static constexpr int SCORE_OPEN_2 = 500;
    static constexpr int SCORE_CLOSED_2 = 50;
    static constexpr int SCORE_BASE = 10;

    struct MoveScore {
        int x;
        int y;
        int score;
    };

    std::vector<MoveScore> get_candidate_moves(const State& state, Sign my_sign) const;
    int evaluate_board(const State& state, Sign my_sign) const;
    int minimax(State state, int depth, int alpha, int beta, bool maximizingPlayer, Sign my_sign, Sign current_turn) const;
    MoveScore get_best_move(const State& state, Sign my_sign) const;

private:
    bool has_stone_nearby(const State& state, int cx, int cy, int rows, int cols) const;
    int evaluate_cell(const State& state, int cx, int cy, Sign target_sign) const;
    int evaluate_direction(const State& state, int cx, int cy, int dx, int dy, 
                           Sign target_sign, int rows, int cols, int win_len) const;
};

class MyPlayer : public IPlayer {
  Sign m_sign = Sign::NONE;
  const char *m_name;

public:
  MyPlayer(const char *name) : m_sign(Sign::NONE), m_name(name) {}
  void set_sign(Sign sign) override;
  Point make_move(const State &game) override;
  const char *get_name() const override;
};

}; // namespace ttt::my_player
