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

int main() {
    std::cout << "Running make_move unit tests...\n";
    test_first_move_takes_center();
    test_immediate_win();
    test_block_opponent_win();
    test_block_open_four();
    std::cout << "All make_move tests passed!\n";
    return 0;
}
