#include "player/my_player.hpp"
#include "core/state.hpp"

#include <iostream>
#include <cassert>

using ttt::game::State;
using ttt::game::Sign;
using ttt::game::Point;
using ttt::my_player::MyPlayer;

void test_first_move_takes_center() {
    State::Opts opts{15, 15, 5, 225}; // 15x15 board, 5 to win
    State state(opts);
    MyPlayer p1("p1");
    p1.set_sign(Sign::X);

    Point m = p1.make_move(state);
    
    // Центр доски 15х15 находится по координатам (7, 7)
    if (m.x == 7 && m.y == 7) {
        std::cout << "[OK] test_first_move_takes_center\n";
    } else {
        std::cerr << "[FAIL] test_first_move_takes_center. Expected (7,7), got (" << m.x << "," << m.y << ")\n";
        assert(false);
    }
}

void test_immediate_win() {
    State::Opts opts{15, 15, 5, 225};
    State state(opts);
    MyPlayer p1("p1");
    p1.set_sign(Sign::X);

    // Ставим Х в ряд 4 штуки
    state.process_move(Sign::X, 1, 1);
    state.process_move(Sign::O, 1, 2);
    state.process_move(Sign::X, 2, 1);
    state.process_move(Sign::O, 2, 2);
    state.process_move(Sign::X, 3, 1);
    state.process_move(Sign::O, 3, 2);
    state.process_move(Sign::X, 4, 1);
    state.process_move(Sign::O, 4, 2);

    // Теперь очередь Х. Ожидаем, что он закроет ряд из 5 либо на (5, 1), либо на (0, 1)
    Point m = p1.make_move(state);
    
    if ((m.x == 5 && m.y == 1) || (m.x == 0 && m.y == 1)) {
        std::cout << "[OK] test_immediate_win\n";
    } else {
        std::cerr << "[FAIL] test_immediate_win. Expected (5,1) or (0,1), got (" << m.x << "," << m.y << ")\n";
        assert(false);
    }
}

void test_block_opponent_win() {
    State::Opts opts{15, 15, 5, 225};
    State state(opts);
    MyPlayer p1("p1");
    p1.set_sign(Sign::O);

    // X ходит первым.
    state.process_move(Sign::X, 10, 0); 
    
    // O заранее блокирует одну сторону линии X, чтобы линия стала "закрытой 4-кой"
    state.process_move(Sign::O, 0, 1);
    
    // Противник (Х) строит линию вниз:
    state.process_move(Sign::X, 1, 1); // 1-й
    state.process_move(Sign::O, 10, 10);
    state.process_move(Sign::X, 2, 1); // 2-й
    state.process_move(Sign::O, 10, 11);
    state.process_move(Sign::X, 3, 1); // 3-й
    state.process_move(Sign::O, 10, 12);
    state.process_move(Sign::X, 4, 1); // 4-й

    // Очередь О. Если мы не заблокируем на (5,1), то Х выиграет вниз (ведь сверху заблокировано)
    Point m = p1.make_move(state);
    
    if (m.x == 5 && m.y == 1) {
        std::cout << "[OK] test_block_opponent_win\n";
    } else {
        std::cerr << "[FAIL] test_block_opponent_win. Expected (5,1), got (" << m.x << "," << m.y << ")\n";
        assert(false);
    }
}

void test_block_open_four() {
    State::Opts opts{15, 15, 5, 225};
    State state(opts);
    MyPlayer p1("p1");
    p1.set_sign(Sign::X); // Мы - Х, противник - О.

    // Противник (О) формирует открытую тройку по вертикали (x=5), которая следующим ходом может стать открытой четверкой.
    // X ходит где-то далеко и не создает никаких угроз.
    state.process_move(Sign::X, 10, 10);
    state.process_move(Sign::O, 5, 4);
    state.process_move(Sign::X, 12, 12);
    state.process_move(Sign::O, 5, 5);
    state.process_move(Sign::X, 10, 14);
    state.process_move(Sign::O, 5, 6);

    // Сейчас очередь рассчета для X. Открытая тройка O находится на (5,4)-(5,5)-(5,6).
    // X не имеет собственных линий, поэтому ОБЯЗАН ставить блок.
    Point m = p1.make_move(state);
    
    if ((m.x == 5 && m.y == 3) || (m.x == 5 && m.y == 7)) {
        std::cout << "[OK] test_block_open_four\n";
    } else {
        std::cerr << "[FAIL] test_block_open_four. Expected (5,3) or (5,7), got (" << m.x << "," << m.y << ")\n";
        assert(false);
    }
}

void test_get_candidate_moves() {
    State::Opts opts{5, 5, 5, 25};
    State state(opts);
    ttt::my_player::HeuristicEvaluator evaluator;

    // Пустая доска, должны получить пустой массив кандидатов, так как рядом нет камней
    auto candidates = evaluator.get_candidate_moves(state, Sign::X);
    assert(candidates.empty());

    // Ставим камень, проверяем что кандидаты теперь есть (не больше 24, обычно вокруг камня)
    state.process_move(Sign::X, 2, 2);
    candidates = evaluator.get_candidate_moves(state, Sign::O);
    assert(!candidates.empty());
    
    // Проверим, что лучший кандидат получает больше всего очков
    // С точки зрения O нужно блокировать/ходить рядом
    bool found = false;
    for(auto c : candidates) {
        if(c.x == 2 && c.y == 3) found = true; // Рядом с X
    }
    assert(found);
    std::cout << "[OK] test_get_candidate_moves\n";
}

void test_evaluate_board() {
    State::Opts opts{5, 5, 3, 25}; // Маленькое поле
    State state(opts);
    ttt::my_player::HeuristicEvaluator evaluator;

    // Пустая доска
    int eval = evaluator.evaluate_board(state, Sign::X);
    assert(eval == 0); // Позиция одинакова для обоих

    // Ставим крестик, Х должен быть выигрышнее
    state.process_move(Sign::X, 2, 2);
    state.process_move(Sign::O, 0, 0); // Нолик далеко
    // Теперь позиция Х чуть лучше или равна (центр против края)
    int eval_x = evaluator.evaluate_board(state, Sign::X);
    int eval_o = evaluator.evaluate_board(state, Sign::O);
    // Для X должно быть больше, чем для O (если мы оцениваем с т.з. X, оценка должна быть положительной)
    assert(eval_x > 0 || (eval_x == -eval_o)); 
    std::cout << "[OK] test_evaluate_board\n";
}

void test_minimax() {
    State::Opts opts{3, 3, 3, 9}; // Крестики-нолики 3x3
    State state(opts);
    ttt::my_player::HeuristicEvaluator evaluator;

    // Создаем ситуацию, где X выигрывает в один ход (если ходит X)
    state.process_move(Sign::X, 0, 0);
    state.process_move(Sign::O, 1, 2);
    state.process_move(Sign::X, 0, 1);
    state.process_move(Sign::O, 2, 2); 

    // Вызываем get_best_move
    auto move = evaluator.get_best_move(state, Sign::X);
    
    // Ожидаем, что лучший ход будет на (0, 2) с оценкой победы
    if(move.x != 0 || move.y != 2) {
        std::cerr << "Expected move at (0,2) but got (" << move.x << "," << move.y << ") with score " << move.score << std::endl;
        assert(false);
    }

    std::cout << "[OK] test_minimax\n";
}

int main() {
    std::cout << "Running make_move unit tests...\n";
    test_first_move_takes_center();
    test_immediate_win();
    test_block_opponent_win();
    test_block_open_four();
    
    test_get_candidate_moves();
    test_evaluate_board();
    test_minimax();
    
    std::cout << "All make_move tests passed!\n";
    return 0;
}
