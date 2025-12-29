/**
 * 文件名: GameCore.h
 * 职责: 定义游戏的核心数据结构和接口 (The Blueprint)
 * 作者: 架构师 (你)
 */

#ifndef GAME_CORE_H // 防止重复引用
#define GAME_CORE_H

#include <iostream>
#include <string>
#include <vector>
#include <algorithm> // 用于 std::max

// 命名空间管理，避免冲突
using namespace std;

// 装备稀有度
enum Rarity { BROKEN, STANDARD, MILITARY, LEGENDARY };

// [基类] 装备
// 这是一个抽象基类 (Abstract Base Class)，不能直接实例化
class Equipment {
protected:
    string name;      // 装备名称
    Rarity rarity;    // 稀有度
    int level;        // 等级
    string faction;   // 势力
    int tid;
public:
    // 构造函数
    Equipment(int id,string n, Rarity r, int lv, string fac): tid(id), name(n), rarity(r), level(lv), faction(fac) {}
    int getId() const { return tid; }
    // 虚析构函数 (重要：父类必须有虚析构，否则子类内存无法释放)
    virtual ~Equipment();

    // --- 纯虚函数接口 (组员必须在子类中实现) ---
    virtual int calculatePower() const = 0;      // 计算战斗力
    virtual string getDescription() const = 0;   // 获取描述文本
    
    // --- 核心逻辑接口 (由架构师实现) ---
    string getName() const;
    int getLevel() const;
    Rarity getRarity() const;
    
    // 运算符重载：实现"合成"功能
    // 声明：两个 Equipment 指针的内容相加，返回一个新的 Equipment 指针
    Equipment* operator+(const Equipment& other);

    // 原型模式：用于克隆对象，辅助合成
    virtual Equipment* clone(string newName, int newLv) const = 0;
    
    // 升级系统接口
    virtual bool canLevelUp() const = 0;
    virtual int getUpgradeCost() const = 0;
    virtual void levelUp() = 0;
    virtual int getMaxLevel() const { return 3; }
};

// 武器类
class Weapon : public Equipment {
private:
    int baseAtk;       // 基础攻击力（1级时的值）
    int baseCritRate;  // 基础暴击概率
    int baseAtkSpeed;  // 基础攻击速度
    int weight;        // 重量（不受等级影响）

    // 根据等级计算实际属性
    int getActualAtk() const {
        return static_cast<int>(baseAtk * (1.0 + 0.2 * (level - 1)));
    }
    
    int getActualCritRate() const {
        int rate = static_cast<int>(baseCritRate * (1.0 + 0.2 * (level - 1)));
        return rate > 100 ? 100 : rate;  // 不超过100%
    }
    
    int getActualAtkSpeed() const {
        // 攻击速度降低（越小越快）
        int speed = static_cast<int>(baseAtkSpeed / (1.0 + 0.2 * (level - 1)));
        return speed < 1 ? 1 : speed;  // 最快1回合
    }

public:
    Weapon(int id, string n, Rarity r, int lv, string fac, int attack, int crit, int speed, int w)
        : Equipment(id, n, r, lv, fac), baseAtk(attack), baseCritRate(crit), baseAtkSpeed(speed), weight(w) {}

    // Getter 方法 - 返回实际值
    int getAtk() const { return getActualAtk(); }
    int getCritRate() const { return getActualCritRate(); }
    int getAtkSpeed() const { return getActualAtkSpeed(); }
    int getWeight() const { return weight; }
    
    // 获取基础属性（用于存档）
    int getBaseAtk() const { return baseAtk; }
    int getBaseCritRate() const { return baseCritRate; }
    int getBaseAtkSpeed() const { return baseAtkSpeed; }
    
    // 升级相关
    int getMaxLevel() const { return 3; }
    bool canLevelUp() const { return level < getMaxLevel(); }
    
    int getUpgradeCost() const {
        // 升级消耗 = 10 * 稀有度等级
        int rarityLevel = static_cast<int>(rarity) + 1;  // BROKEN=1, STANDARD=2, MILITARY=3, LEGENDARY=4
        return 10 * rarityLevel;
    }
    
    void levelUp() {
        if (canLevelUp()) {
            level++;
        }
    }

    // 实现基类的纯虚函数
    int calculatePower() const override {
        // 战斗力公式：攻击力 * (1 + 暴击率/200) * (10/攻击速度) * 等级
        return static_cast<int>(getActualAtk() * (1.0 + getActualCritRate()/200.0) * (10.0/getActualAtkSpeed()) * level);
    }

    string getDescription() const override {
        return "[武器] 攻击: " + to_string(getActualAtk()) + 
               " | 暴击率: " + to_string(getActualCritRate()) + "%" +
               " | 速度: " + to_string(getActualAtkSpeed()) + "回合/次" +
               " | 重量: " + to_string(weight) +
               " | 势力: " + faction;
    }

    // 实现克隆，用于合成
    Equipment* clone(string newName, int newLv) const override {
        // 合成后攻击力提升 50%
        return new Weapon(this->tid, newName, this->rarity, newLv, this->faction, 
                         this->baseAtk * 1.5, this->baseCritRate, this->baseAtkSpeed, this->weight);
    }
};

// 装甲类
class Armor : public Equipment {
private:
    int baseMaxHp;     // 基础血量上限（1级时的值）
    int baseDodgeRate; // 基础完美闪避概率
    int baseCapacity;  // 基础总承重量

    // 根据等级计算实际属性
    int getActualMaxHp() const {
        return static_cast<int>(baseMaxHp * (1.0 + 0.2 * (level - 1)));
    }
    
    int getActualDodgeRate() const {
        int rate = static_cast<int>(baseDodgeRate * (1.0 + 0.2 * (level - 1)));
        return rate > 100 ? 100 : rate;  // 不超过100%
    }
    
    int getActualCapacity() const {
        return static_cast<int>(baseCapacity * (1.0 + 0.2 * (level - 1)));
    }

public:
    Armor(int id, string n, Rarity r, int lv, string fac, int hp, int dodge, int cap)
        : Equipment(id, n, r, lv, fac), baseMaxHp(hp), baseDodgeRate(dodge), baseCapacity(cap) {}

    // Getter 方法 - 返回实际值
    int getMaxHp() const { return getActualMaxHp(); }
    int getDodgeRate() const { return getActualDodgeRate(); }
    int getCapacity() const { return getActualCapacity(); }
    
    // 获取基础属性（用于存档）
    int getBaseMaxHp() const { return baseMaxHp; }
    int getBaseDodgeRate() const { return baseDodgeRate; }
    int getBaseCapacity() const { return baseCapacity; }
    
    // 升级相关
    int getMaxLevel() const { return 3; }
    bool canLevelUp() const { return level < getMaxLevel(); }
    
    int getUpgradeCost() const {
        // 升级消耗 = 10 * 稀有度等级
        int rarityLevel = static_cast<int>(rarity) + 1;  // BROKEN=1, STANDARD=2, MILITARY=3, LEGENDARY=4
        return 10 * rarityLevel;
    }
    
    void levelUp() {
        if (canLevelUp()) {
            level++;
        }
    }

    int calculatePower() const override {
        // 战斗力公式：血量 / 10 + 闪避率 * 2 + 承重量 + 等级 * 5
        return getActualMaxHp() / 10 + getActualDodgeRate() * 2 + getActualCapacity() + level * 5;
    }

    string getDescription() const override {
        return "[装甲] 血量: " + to_string(getActualMaxHp()) + 
               " | 闪避率: " + to_string(getActualDodgeRate()) + "%" +
               " | 承重: " + to_string(getActualCapacity()) +
               " | 势力: " + faction;
    }

    Equipment* clone(string newName, int newLv) const override {
        return new Armor(this->tid, newName, this->rarity, newLv, this->faction, 
                        this->baseMaxHp + 200, this->baseDodgeRate, this->baseCapacity + 5);
    }
};

#endif