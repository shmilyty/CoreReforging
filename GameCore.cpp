/**
 * 文件名: GameCore.cpp
 * 职责: 实现 GameCore.h 中定义的复杂逻辑
 * 作者: 架构师 (你)
 */

#include "GameCore.h"
#include <fstream>
#include "json.hpp" // 必须确保这个文件在同级目录
using json = nlohmann::json;
// 构造函数实现已在头文件中内联实现

// 析构函数实现 (即使为空也需要写出来)
Equipment::~Equipment() {}

// 获取名字
string Equipment::getName() const {
    return name;
}

// 获取等级
int Equipment::getLevel() const {
    return level;
}

// 获取稀有度
Rarity Equipment::getRarity() const {
    return rarity;
}

// [重点] 合成逻辑的实现
// 这里演示了如何利用多态和 clone 来生成新对象
Equipment* Equipment::operator+(const Equipment& other) {
    // 逻辑：新等级 = 两者最大等级 + 1
    int newLevel = max(this->level, other.level) + 1;
    
    // 逻辑：新名字 = 加上前缀
    string newName = "强化型-" + this->name; 
    
    // 调用子类的 clone 方法生成新对象
    // 注意：这里的 *this 代表当前对象，我们调用自己的 clone
    return this->clone(newName, newLevel); 
}