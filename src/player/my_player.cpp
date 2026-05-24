#include "my_player.hpp"
#include <cstdlib>
#include <chrono>
#include <vector>
#include <algorithm>

namespace ttt::my_player {

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

    // -------------------------------------------------------------------------
    // get_candidate_moves:
    // Функция генерирует список наиболее перспективных ходов. 
    // Зачем это нужно? Минимакс - очень затратный алгоритм. Если мы будем 
    // проверять вообще все пустые клетки на поле , обсчет займет часы. Поэтому этот метод отсекает "мусорные" 
    // ходы и готовит сортированный список лучших кандидатов для минимакса.
    // -------------------------------------------------------------------------
    std::vector<MoveScore> get_candidate_moves(const State& state, Sign my_sign) const {
        int rows = state.get_opts().rows;
        int cols = state.get_opts().cols;
        Sign opp_sign = (my_sign == Sign::X) ? Sign::O : Sign::X;

        std::vector<MoveScore> candidates;
        for (int y = 0; y < rows; ++y) {
            for (int x = 0; x < cols; ++x) {
                if (state.get_value(x, y) == Sign::NONE) {
                    // Эвристика №1: Не рассматриваем клетки, рядом с которыми
                    // в радиусе 2х клеток нет ни одного поставленного камня.
                    // Клетки "в чистом поле" почти никогда не бывают хорошим ходом.
                    if (!has_stone_nearby(state, x, y, rows, cols)) {
                        continue;
                    }

                    // Оцениваем, сколько очков клетка принесет нам (построение наших линий)
                    int my_score = evaluate_cell(state, x, y, my_sign);
                    // Оцениваем, сколько очков клетка принесет противнику (блокирование его линий)
                    int opp_score = evaluate_cell(state, x, y, opp_sign);

                    // Итоговая базовая "горячесть" поля - сумма атаки и обороны
                    int total_score = my_score + opp_score;
                    
                    // Экстренная защита:
                    // Если противник может выиграть своим следующим ходом (SCORE_WIN),
                    // мы искусственно увеличиваем приоритет этой клетки до гигантских
                    // 2 миллионов, чтобы алгоритм 100% рассмотрел этот ход первым и заблокировал.
                    if (opp_score >= SCORE_WIN) {
                        total_score += 2000000; 
                    } 
                    // Если противник может собрать открытую 4-ку (которую потом нельзя закрыть)
                    // мы тоже завышаем приоритет.
                    else if (opp_score >= SCORE_OPEN_4) {
                        total_score += 100000;  
                    }

                    candidates.push_back({x, y, total_score});
                }
            }
        }
        // Эвристика №2 для Альфа-Бета отсечения:
        // Альфа-бета отсечение работает в разы быстрее, если мы сначала проверяем 
        // самые "мощные" ходы, так как они быстрее отсекают "слабые" ветки.
        // Поэтому сортируем кандидатов по убыванию.
        std::sort(candidates.begin(), candidates.end(), [](const MoveScore& a, const MoveScore& b) {
            return a.score > b.score;
        });
        return candidates;
    }

    // -------------------------------------------------------------------------
    // evaluate_board:
    // Функция оценки ВСЕЙ доски. Она вызывается, когда Минимакс доходит до 
    // максимальной глубины (depth == 0) и нужно дать грубую оценку текущей ситуации.
    // -------------------------------------------------------------------------
    int evaluate_board(const State& state, Sign my_sign) const {
        int rows = state.get_opts().rows;
        int cols = state.get_opts().cols;
        Sign opp_sign = (my_sign == Sign::X) ? Sign::O : Sign::X;
        
        int my_total = 0;
        int opp_total = 0;
        
        for (int y = 0; y < rows; ++y) {
            for (int x = 0; x < cols; ++x) {
                // Чтобы не тратить время на оценку пустых участков доски,
                // мы считаем оценку только вокруг уже поставленных камней.
                if (state.get_value(x, y) == Sign::NONE && has_stone_nearby(state, x, y, rows, cols)) {
                    my_total += evaluate_cell(state, x, y, my_sign);
                    // Оцениваем силу оппонента и вычитаем из нашей оценки
                    opp_total += evaluate_cell(state, x, y, opp_sign);
                }
            }
        }
        // Чем больше разница между нашей силой и силой противника, 
        // тем выгоднее эта доска для нас.
        return my_total - opp_total;
    }

    // -------------------------------------------------------------------------
    // minimax:
    // Классический рекурсивный Минимакс алгоритм с Альфа-Бета отсечением.
    // Пытается заглянуть в будущее на depth ходов.
    // -------------------------------------------------------------------------
    int minimax(State state, int depth, int alpha, int beta, bool maximizingPlayer, Sign my_sign, Sign current_turn) const {
        // 1. Условие выхода (Конец Игры)
        if (state.get_status() == game::Status::ENDED) {
            Sign winner = state.get_winner();
            // Выиграли мы: даем максимум очков, плюс бонус за быструю победу (чем больше глубина осталась, тем быстрее победа)
            if (winner == my_sign) return 100000000 + depth; 
            // Ничья: доска нас полностью не устраивает, но и не катастрофа
            if (winner == Sign::NONE) return 0; 
            // Выиграл враг: наказываем (штраф тем больше, чем быстрее победил противник)
            return -100000000 - depth; 
        }
        
        // 2. Условие выхода (Достигнут предел поиска)
        if (depth == 0) {
            // Если игра не закончена, но мы достигли заданного предела ходов,
            // возвращаем примерную эвристическую оценку текущего поля.
            return evaluate_board(state, my_sign);
        }

        Sign next_turn = (current_turn == Sign::X) ? Sign::O : Sign::X;
        auto candidates = get_candidate_moves(state, current_turn);
        
        // ОГРАНИЧЕНИЕ ШИРИНЫ (Beam Search):
        // Ограничиваем ветвление топ-8 ходами! Если мы будем перебирать
        // даже 30 кандидатов на глубину 3, это 30 * 30 * 30 = 27000 досок.
        // Ограничение до 8: 8 * 8 * 8 = 512 досок (в 50 раз быстрее).
        int max_candidates = 8; 
        if (candidates.size() > max_candidates) {
            candidates.resize(max_candidates);
        }

        // Если кандидатов нет вообще (редкий крайний случай пустой доски),
        if (candidates.empty()) {
            return evaluate_board(state, my_sign);
        }

        // 3. Шаг Максимизирующего игрока (НАШ ХОД)
        if (maximizingPlayer) {
            int maxEval = -2000000000;
            for (const auto& move : candidates) {
                // Симулируем ход
                State next_state = state;
                next_state.process_move(current_turn, move.x, move.y);
                
                // Спускаемся на уровень ниже, но теперь ходит соперник (minimizingPlayer=false)
                int eval = minimax(next_state, depth - 1, alpha, beta, false, my_sign, next_turn);
                
                maxEval = std::max(maxEval, eval);
                
                // Альфа хранит наилучшую (максимальную) оценку из ветвей для максимизирующего игрока
                alpha = std::max(alpha, eval);
                
                // Отсечение: если противник на уровне выше (Beta) уже нашел вариант
                // с оценкой МЕНЬШЕ, чем мы нашли тут (Alpha), он никогда не пойдет сюда.
                // Значит, нам нет смысла дальше считать.
                if (beta <= alpha) break; 
            }
            return maxEval;
        } 
        // 4. Шаг Минимизирующего игрока (ХОД ВРАГА)
        else {
            // Враг пытается минимизировать нашу общую оценку
            int minEval = 2000000000;
            for (const auto& move : candidates) {
                // Симулируем ход врага
                State next_state = state;
                next_state.process_move(current_turn, move.x, move.y);
                
                // Спускаемся на уровень ниже, теперь снова наш ход (maximizingPlayer=true)
                int eval = minimax(next_state, depth - 1, alpha, beta, true, my_sign, next_turn);
                
                minEval = std::min(minEval, eval);
                
                // Бета хранит наилучшую для противника (минимальную для нас) оценку
                beta = std::min(beta, eval);
                
                // Отсечение
                if (beta <= alpha) break; 
            }
            return minEval;
        }
    }

    // -------------------------------------------------------------------------
    // get_best_move:
    // Стартовая функция (Root node), принимающая текущее состояние игры.
    // Работает как "нулевой" уровень глубины Минимакса.
    // -------------------------------------------------------------------------
    MoveScore get_best_move(const State& state, Sign my_sign) const {
        int rows = state.get_opts().rows;
        int cols = state.get_opts().cols;
        Sign opp_sign = (my_sign == Sign::X) ? Sign::O : Sign::X;

        // Оптимизация первого хода: если доска пуста и центр свободен - забираем центр.
        // Экономит время и является 100% оптимальным первым ходом на любой доске.
        if (state.get_move_no() == 0 && state.get_value(cols / 2, rows / 2) == Sign::NONE) {
            return {cols / 2, rows / 2, SCORE_BASE};
        }

        // Получаем отсортированный список лучших стартовых ходов
        auto candidates = get_candidate_moves(state, my_sign);
        
        if (candidates.empty()) {
             // Fallback если вдруг все клетки рядом с камнями заняты 
             // (бывает только при старте на гигантском поле или специфичных инициализаторах)
             for (int y = 0; y < rows; ++y) {
                  for (int x = 0; x < cols; ++x) {
                       if (state.get_value(x, y) == Sign::NONE) {
                            return {x, y, 0};
                       }
                  }
             }
        }

        // Ограничиваем стартовое окно поиска (Top-8)
        int max_candidates = 8;
        if (candidates.size() > max_candidates) {
            candidates.resize(max_candidates);
        }

        // DEBUG
        // for(auto c : candidates) std::cout << "Cand: " << c.x << "," << c.y << " score=" << c.score << "\n";

        MoveScore best_move{-1, -1, -2000000000};
        
        // Переменные для Альфа-Бета окна начального узла
        int alpha = -2000000000;
        int beta = 2000000000;

        for (const auto& move : candidates) {
            State next_state = state;
            next_state.process_move(my_sign, move.x, move.y);

            // Проверка немедленной победы на самом первом шаге.
            // Если ход побеждает, сразу берем его и выходим.
            if (next_state.get_status() == game::Status::ENDED && next_state.get_winner() == my_sign) {
                return {move.x, move.y, 2000000000};
            }

            // Запускаем минимакс с глубиной 2. 
            // Этот главный цикл for выступает в роли "еще одной" глубины (Depth 1), 
            // так что итоговая глубина = 1 (в цикле) + 2 (в минимаксе) = 3 шага вперед.
            int eval = minimax(next_state, 2, alpha, beta, false, my_sign, opp_sign);
            
            // Если найден лучший вариант, обновляем лучший ход
            if (eval > best_move.score) {
                best_move = {move.x, move.y, eval};
            }
            // Подвязываем верхний уровень Альфа к оценке
            if (eval > alpha) {
                alpha = eval;
            }
        }

        // Страховочный возврат, если вдруг оценки сломались (никогда не должно произойти)
        if (best_move.x == -1 && !candidates.empty()) {
            return candidates[0];
        }

        return best_move;
    }

private:
    bool has_stone_nearby(const State& state, int cx, int cy, int rows, int cols) const {
        for (int dy = -2; dy <= 2; ++dy) {
            for (int dx = -2; dx <= 2; ++dx) {
                if (dx == 0 && dy == 0) continue;
                int nx = cx + dx;
                int ny = cy + dy;
                if (nx >= 0 && nx < cols && ny >= 0 && ny < rows) {
                    Sign s = state.get_value(nx, ny);
                    if (s == Sign::X || s == Sign::O) return true;
                }
            }
        }
        // Если рядом нет камней, считаем эту клетку неинтересной для хода
        return false; 
    }

    int evaluate_cell(const State& state, int cx, int cy, Sign target_sign) const {
        int total_score = 0;
        const int dx[] = {1, 0, 1, 1};
        const int dy[] = {0, 1, 1, -1};
        int rows = state.get_opts().rows;
        int cols = state.get_opts().cols;

        for (int dir = 0; dir < 4; ++dir) {
            total_score += evaluate_direction(state, cx, cy, dx[dir], dy[dir], target_sign, rows, cols);
        }
        return total_score;
    }

    int evaluate_direction(const State& state, int cx, int cy, int dx, int dy, 
                           Sign target_sign, int rows, int cols) const {
        int line[9];
        
        for (int i = -4; i <= 4; ++i) {
            int nx = cx + i * dx;
            int ny = cy + i * dy;
            
            if (i == 0) {
                line[i + 4] = 1; 
            } else if (nx >= 0 && nx < cols && ny >= 0 && ny < rows) {
                Sign s = state.get_value(nx, ny);
                if (s == target_sign) line[i + 4] = 1;           
                else if (s == Sign::NONE) line[i + 4] = 0; 
                else line[i + 4] = 2;                            
            } else {
                line[i + 4] = 2; 
            }
        }

        int w5 = 0, w4 = 0, w3 = 0, w2 = 0;
        
        for (int start_idx = 0; start_idx <= 4; ++start_idx) {
            bool impossible = false;
            int count_1 = 0;
            
            for (int j = 0; j < 5; ++j) {
                if (line[start_idx + j] == 2) { 
                    impossible = true; 
                    break;
                }
                if (line[start_idx + j] == 1) count_1++;
            }
            
            if (impossible) continue; 
            
            if (count_1 == 5) w5++;
            else if (count_1 == 4) w4++;
            else if (count_1 == 3) w3++;
            else if (count_1 == 2) w2++;
        }

        if (w5 > 0) return SCORE_WIN;
        if (w4 >= 2) return SCORE_OPEN_4;
        if (w4 == 1) return SCORE_CLOSED_4; 
        if (w3 >= 2) return SCORE_OPEN_3;
        if (w3 == 1) return SCORE_CLOSED_3;
        if (w2 >= 2) return SCORE_OPEN_2;
        if (w2 == 1) return SCORE_CLOSED_2;

        return SCORE_BASE; 
    }
};

void MyPlayer::set_sign(Sign sign) { m_sign = sign; }
const char *MyPlayer::get_name() const { return m_name; }

Point MyPlayer::make_move(const State &state) {
  HeuristicEvaluator evaluator;
  auto best = evaluator.get_best_move(state, m_sign);
  Point result;
  result.x = best.x;
  result.y = best.y;
  return result;
}

}; // namespace ttt::my_player
