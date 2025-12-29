/**
 * 文件名: Adventure.h
 * 职责: 冒险系统 - 战斗、篝火、难度递增
 */

#ifndef ADVENTURE_H
#define ADVENTURE_H

#include <iostream>
#include <vector>
#include <random>
#include <ctime>
#include <windows.h>
#include "GameCore.h"

using namespace std;

// Monster 结构体定义（与 DataLoader.h 中相同）
#ifndef MONSTER_STRUCT_DEFINED
#define MONSTER_STRUCT_DEFINED
struct Monster {
    int id;
    string name;
    int hp;
    int atk;
    int exp;
};
#endif

// EquipmentSlot 结构体定义（与 main.cpp 中相同）
#ifndef EQUIPMENT_SLOT_DEFINED
#define EQUIPMENT_SLOT_DEFINED
struct EquipmentSlot {
    Armor* equippedArmor;
    vector<Weapon*> equippedWeapons;
    
    EquipmentSlot() : equippedArmor(nullptr) {}
    
    int getTotalWeight() const {
        int total = 0;
        for (auto w : equippedWeapons) {
            if (w) total += w->getWeight();
        }
        return total;
    }
    
    int getEffectiveDodgeRate() const {
        if (!equippedArmor) return 0;
        int capacity = equippedArmor->getCapacity();
        int weight = getTotalWeight();
        // 如果重量超过60%承重，闪避率强制为0
        if (weight > capacity * 0.6) return 0;
        return equippedArmor->getDodgeRate();
    }
};
#endif

// 战斗统计
struct WeaponStats {
    string weaponName;
    int totalDamage;
    int hits;
    
    WeaponStats(string name) : weaponName(name), totalDamage(0), hits(0) {}
};

// 冒险统计
struct AdventureStats {
    int totalExpGained;
    int totalExpSpent;
    int enemiesDefeated;
    int campfiresReached;
    vector<WeaponStats> weaponStats;
    
    AdventureStats() : totalExpGained(0), totalExpSpent(0), enemiesDefeated(0), campfiresReached(0) {}
    
    void addWeaponDamage(const string& weaponName, int damage) {
        for (auto& ws : weaponStats) {
            if (ws.weaponName == weaponName) {
                ws.totalDamage += damage;
                ws.hits++;
                return;
            }
        }
        // 如果武器不存在，添加新的
        WeaponStats newStats(weaponName);
        newStats.totalDamage = damage;
        newStats.hits = 1;
        weaponStats.push_back(newStats);
    }
};

// 冒险系统类
class AdventureSystem {
private:
    vector<Monster> allMonsters;
    EquipmentSlot* playerEquipment;
    int& playerExp;
    int playerCurrentHp;
    int playerMaxHp;
    AdventureStats stats;
    int difficultyLevel;  // 难度等级（经过的篝火数）
    int battlesUntilCampfire;  // 距离下一个篝火的战斗数
    
    // 随机数生成器
    mt19937 rng;
    
    // 获取随机怪物
    Monster getRandomMonster() {
        uniform_int_distribution<int> dist(0, allMonsters.size() - 1);
        Monster monster = allMonsters[dist(rng)];
        
        // 根据难度调整怪物属性
        double multiplier = 1.0 + (difficultyLevel * 0.05);
        monster.hp = static_cast<int>(monster.hp * multiplier);
        monster.atk = static_cast<int>(monster.atk * multiplier);
        monster.exp = static_cast<int>(monster.exp * (1.0 + difficultyLevel * 0.1));
        
        return monster;
    }
    
    // 计算玩家总攻击力
    int calculatePlayerAttack(Weapon* weapon) {
        if (!weapon) return 0;
        
        int damage = weapon->getAtk();
        
        // 暴击判定
        uniform_int_distribution<int> critDist(1, 100);
        if (critDist(rng) <= weapon->getCritRate()) {
            damage *= 2;
            cout << "  【暴击！】伤害翻倍！" << endl;
        }
        
        return damage;
    }
    
    // 显示当前冒险状态（包括临时EXP）
    void showAdventureStatus() {
        cout << "\n【冒险状态】" << endl;
        cout << "  当前 HP: " << playerCurrentHp << "/" << playerMaxHp << endl;
        cout << "  基地 EXP: " << playerExp << endl;
        cout << "  本次冒险已获得: " << stats.totalExpGained << " EXP (结束后返还)" << endl;
        cout << "  本次冒险已消耗: " << stats.totalExpSpent << " EXP" << endl;
        cout << "  预计净收益: " << (stats.totalExpGained - stats.totalExpSpent) << " EXP" << endl;
    }
    
    // 战斗回合
    bool battleRound(Monster& monster) {
        system("cls");
        cout << "\n=== 战斗中 ===" << endl;
        cout << "玩家 HP: " << playerCurrentHp << "/" << playerMaxHp << endl;
        cout << monster.name << " HP: " << monster.hp << endl;
        cout << "===================" << endl;
        
        // 玩家回合 - 所有装备的武器攻击
        if (!playerEquipment->equippedWeapons.empty()) {
            cout << "\n【玩家回合】" << endl;
            for (auto weapon : playerEquipment->equippedWeapons) {
                if (!weapon) continue;  // 安全检查
                
                int damage = calculatePlayerAttack(weapon);
                monster.hp -= damage;
                stats.addWeaponDamage(weapon->getName(), damage);
                
                cout << "  使用 " << weapon->getName() << " 造成 " << damage << " 点伤害！" << endl;
                
                if (monster.hp <= 0) {
                    cout << "\n敌人被击败！" << endl;
                    Sleep(1000);
                    return true;
                }
            }
        } else {
            cout << "\n【警告】没有装备武器！无法攻击！" << endl;
            cout << "你将被迫撤退..." << endl;
            Sleep(2000);
            playerCurrentHp = 0;  // 强制失败
            return false;
        }
        
        Sleep(1000);
        
        // 敌人回合
        cout << "\n【敌人回合】" << endl;
        
        // 闪避判定
        int effectiveDodge = 0;
        if (playerEquipment->equippedArmor) {
            effectiveDodge = playerEquipment->getEffectiveDodgeRate();
        }
        
        uniform_int_distribution<int> dodgeDist(1, 100);
        if (dodgeDist(rng) <= effectiveDodge) {
            cout << "  " << monster.name << " 的攻击被完美闪避！" << endl;
        } else {
            playerCurrentHp -= monster.atk;
            cout << "  " << monster.name << " 造成 " << monster.atk << " 点伤害！" << endl;
            
            if (playerCurrentHp <= 0) {
                cout << "\n你被击败了..." << endl;
                Sleep(1000);
                return false;
            }
        }
        
        Sleep(1500);
        return true;  // 继续战斗
    }
    
    // 单场战斗
    bool singleBattle() {
        Monster monster = getRandomMonster();
        
        cout << "\n遭遇敌人：" << monster.name << "！" << endl;
        cout << "敌人属性：HP " << monster.hp << " | 攻击 " << monster.atk << " | EXP " << monster.exp << endl;
        Sleep(1500);
        
        // 战斗循环
        while (true) {
            bool continueResult = battleRound(monster);
            
            if (monster.hp <= 0) {
                // 玩家胜利
                stats.totalExpGained += monster.exp;
                stats.enemiesDefeated++;
                battlesUntilCampfire--;
                
                cout << "\n战斗胜利！获得 " << monster.exp << " EXP！" << endl;
                showAdventureStatus();
                system("pause");
                return true;
            }
            
            if (playerCurrentHp <= 0) {
                // 玩家失败
                return false;
            }
        }
    }
    
    // 篝火处的装备管理
    void campfireEquipmentManage(vector<Equipment*>& inventory) {
        system("cls");
        cout << "\n=== 篝火 - 装备管理 ===" << endl;
        
        // 显示当前装备
        cout << "\n【当前装备】" << endl;
        if (playerEquipment->equippedArmor) {
            cout << "装甲: " << playerEquipment->equippedArmor->getName() << endl;
        } else {
            cout << "装甲: 未装备" << endl;
        }
        
        cout << "武器: ";
        if (playerEquipment->equippedWeapons.empty()) {
            cout << "未装备" << endl;
        } else {
            for (auto w : playerEquipment->equippedWeapons) {
                cout << w->getName() << " ";
            }
            cout << endl;
        }
        
        cout << "\n[1] 更换装甲" << endl;
        cout << "[2] 装备武器" << endl;
        cout << "[3] 卸下武器" << endl;
        cout << "[0] 返回" << endl;
        cout << ">>> 请选择: ";
        
        int choice;
        cin >> choice;
        
        if (choice == 1) {
            // 更换装甲
            cout << "\n可用装甲：" << endl;
            vector<Armor*> armors;
            for (auto item : inventory) {
                if (Armor* armor = dynamic_cast<Armor*>(item)) {
                    armors.push_back(armor);
                }
            }
            
            for (size_t i = 0; i < armors.size(); i++) {
                cout << "[" << i << "] " << armors[i]->getName() 
                     << " (HP:" << armors[i]->getMaxHp() 
                     << " 闪避:" << armors[i]->getDodgeRate() << "% "
                     << "承重:" << armors[i]->getCapacity() << ")" << endl;
            }
            
            cout << "请选择装甲编号 (输入-1取消): ";
            int armorChoice;
            cin >> armorChoice;
            
            if (armorChoice >= 0 && armorChoice < (int)armors.size()) {
                playerEquipment->equippedArmor = armors[armorChoice];
                playerMaxHp = armors[armorChoice]->getMaxHp();
                playerCurrentHp = playerMaxHp;  // 更换装甲后恢复满血
                cout << "\n装备成功！生命值已恢复至满！" << endl;
            }
        } else if (choice == 2) {
            // 装备武器
            cout << "\n可用武器：" << endl;
            vector<Weapon*> weapons;
            for (auto item : inventory) {
                if (Weapon* weapon = dynamic_cast<Weapon*>(item)) {
                    weapons.push_back(weapon);
                }
            }
            
            for (size_t i = 0; i < weapons.size(); i++) {
                cout << "[" << i << "] " << weapons[i]->getName() 
                     << " (攻击:" << weapons[i]->getAtk() 
                     << " 暴击:" << weapons[i]->getCritRate() << "% "
                     << "重量:" << weapons[i]->getWeight() << ")" << endl;
            }
            
            cout << "请选择武器编号 (输入-1取消): ";
            int weaponChoice;
            cin >> weaponChoice;
            
            if (weaponChoice >= 0 && weaponChoice < (int)weapons.size()) {
                playerEquipment->equippedWeapons.push_back(weapons[weaponChoice]);
                cout << "\n装备成功！" << endl;
            }
        } else if (choice == 3) {
            // 卸下武器
            if (playerEquipment->equippedWeapons.empty()) {
                cout << "\n未装备武器！" << endl;
            } else {
                cout << "\n已装备的武器：" << endl;
                for (size_t i = 0; i < playerEquipment->equippedWeapons.size(); i++) {
                    cout << "[" << i << "] " << playerEquipment->equippedWeapons[i]->getName() << endl;
                }
                
                cout << "请选择要卸下的武器编号 (输入-1取消): ";
                int unequipChoice;
                cin >> unequipChoice;
                
                if (unequipChoice >= 0 && unequipChoice < (int)playerEquipment->equippedWeapons.size()) {
                    playerEquipment->equippedWeapons.erase(playerEquipment->equippedWeapons.begin() + unequipChoice);
                    cout << "\n卸下成功！" << endl;
                }
            }
        }
        
        system("pause");
    }
    
    // 篝火处的装备升级
    void campfireEquipmentUpgrade(vector<Equipment*>& inventory) {
        system("cls");
        cout << "\n=== 篝火 - 装备升级 ===" << endl;
        showAdventureStatus();
        
        // 计算可用EXP
        int availableExp = playerExp + stats.totalExpGained - stats.totalExpSpent;
        cout << "\n可用 EXP: " << availableExp << endl;
        
        cout << "\n可升级的装备：" << endl;
        for (size_t i = 0; i < inventory.size(); i++) {
            Equipment* equip = inventory[i];
            Weapon* weapon = dynamic_cast<Weapon*>(equip);
            Armor* armor = dynamic_cast<Armor*>(equip);
            
            cout << "[" << i << "] " << equip->getName();
            
            if (weapon) {
                cout << " [武器]";
            } else if (armor) {
                cout << " [装甲]";
            }
            
            cout << " | 等级: " << equip->getLevel() << "/3";
            
            if (equip->canLevelUp()) {
                int cost = equip->getUpgradeCost();
                cout << " | 升级消耗: " << cost << " EXP";
                if (availableExp >= cost) {
                    cout << " [可升级]";
                } else {
                    cout << " [EXP不足]";
                }
            } else {
                cout << " [已满级]";
            }
            cout << endl;
        }
        
        cout << "\n请选择要升级的装备编号 (输入-1取消): ";
        int upgradeChoice;
        cin >> upgradeChoice;
        
        if (upgradeChoice >= 0 && upgradeChoice < (int)inventory.size()) {
            Equipment* selectedEquip = inventory[upgradeChoice];
            
            if (!selectedEquip->canLevelUp()) {
                cout << "\n该装备已达到最高等级！" << endl;
            } else {
                int cost = selectedEquip->getUpgradeCost();
                if (availableExp < cost) {
                    cout << "\nEXP不足！需要 " << cost << " EXP，当前可用 " << availableExp << " EXP。" << endl;
                } else {
                    stats.totalExpSpent += cost;
                    selectedEquip->levelUp();
                    cout << "\n升级成功！" << endl;
                    cout << selectedEquip->getName() << " 已升级到 Lv." << selectedEquip->getLevel() << "！" << endl;
                    cout << "消耗 " << cost << " EXP" << endl;
                    
                    // 如果升级的是当前装备的装甲，更新最大生命值
                    if (selectedEquip == playerEquipment->equippedArmor) {
                        playerMaxHp = playerEquipment->equippedArmor->getMaxHp();
                        playerCurrentHp = playerMaxHp;  // 升级后恢复满血
                        cout << "装甲升级！生命值已恢复至满！" << endl;
                    }
                }
            }
        }
        
        system("pause");
    }
    
    // 篝火休息
    bool campfireRest(vector<Equipment*>& inventory) {
        system("cls");
        stats.campfiresReached++;
        difficultyLevel++;
        
        cout << "\n🔥 ==================== 🔥" << endl;
        cout << "     到达篝火休息点" << endl;
        cout << "🔥 ==================== 🔥" << endl;
        cout << "\n生命值已完全恢复！" << endl;
        playerCurrentHp = playerMaxHp;
        
        showAdventureStatus();
        cout << "\n  已击败: " << stats.enemiesDefeated << " 个敌人" << endl;
        cout << "  难度等级: " << difficultyLevel << " (敌人属性 +" << (difficultyLevel * 5) << "%)" << endl;
        
        while (true) {
            cout << "\n篝火选项：" << endl;
            cout << "[1] 装备管理" << endl;
            cout << "[2] 装备升级" << endl;
            cout << "[3] 继续冒险" << endl;
            cout << "[4] 传送回基地（结束冒险）" << endl;
            cout << ">>> 请选择: ";
            
            int choice;
            cin >> choice;
            
            switch (choice) {
                case 1:
                    // 装备管理
                    campfireEquipmentManage(inventory);
                    break;
                    
                case 2:
                    // 装备升级
                    campfireEquipmentUpgrade(inventory);
                    break;
                    
                case 3:
                    // 继续冒险
                    battlesUntilCampfire = 3;
                    return true;
                    
                case 4:
                    // 返回基地
                    return false;
                    
                default:
                    cout << "无效选项！" << endl;
                    break;
            }
        }
    }
    
    // 修复服务
    void repairService() {
        if (playerCurrentHp >= playerMaxHp) {
            cout << "\n生命值已满，无需修复！" << endl;
            return;
        }
        
        int hpNeeded = playerMaxHp - playerCurrentHp;
        
        cout << "\n=== 修复服务 ===" << endl;
        cout << "当前 HP: " << playerCurrentHp << "/" << playerMaxHp << endl;
        cout << "需要修复: " << hpNeeded << " HP" << endl;
        cout << "修复费用: " << hpNeeded << " EXP (1 HP = 1 EXP)" << endl;
        
        showAdventureStatus();
        
        // 计算可用的总EXP（基地EXP + 本次获得的EXP - 已消耗的EXP）
        int availableExp = playerExp + stats.totalExpGained - stats.totalExpSpent;
        
        if (availableExp < hpNeeded) {
            cout << "\n可用 EXP 不足，无法完全修复！" << endl;
            cout << "可以修复 " << availableExp << " HP" << endl;
        }
        
        cout << "\n是否使用修复服务？" << endl;
        cout << "[1] 完全修复" << endl;
        cout << "[2] 部分修复" << endl;
        cout << "[3] 取消" << endl;
        cout << ">>> 请选择: ";
        
        int choice;
        cin >> choice;
        
        switch (choice) {
            case 1:
                if (availableExp >= hpNeeded) {
                    stats.totalExpSpent += hpNeeded;
                    playerCurrentHp = playerMaxHp;
                    cout << "\n修复完成！HP: " << playerCurrentHp << "/" << playerMaxHp << endl;
                    showAdventureStatus();
                } else {
                    cout << "\nEXP不足！" << endl;
                }
                break;
                
            case 2:
                cout << "\n请输入要修复的 HP 数量: ";
                int repairAmount;
                cin >> repairAmount;
                
                if (repairAmount > hpNeeded) {
                    repairAmount = hpNeeded;
                }
                
                if (repairAmount > availableExp) {
                    repairAmount = availableExp;
                }
                
                if (repairAmount > 0) {
                    stats.totalExpSpent += repairAmount;
                    playerCurrentHp += repairAmount;
                    cout << "\n修复完成！HP: " << playerCurrentHp << "/" << playerMaxHp << endl;
                    showAdventureStatus();
                }
                break;
                
            case 3:
                cout << "\n取消修复。" << endl;
                break;
        }
        
        system("pause");
    }
    
    // 显示冒险统计
    void showAdventureStats() {
        system("cls");
        cout << "\n╔════════════════════════════════════╗" << endl;
        cout << "║       冒险统计报告                 ║" << endl;
        cout << "╚════════════════════════════════════╝" << endl;
        
        cout << "\n【战斗统计】" << endl;
        cout << "  击败敌人: " << stats.enemiesDefeated << " 个" << endl;
        cout << "  到达篝火: " << stats.campfiresReached << " 处" << endl;
        cout << "  最终难度: Lv." << difficultyLevel << endl;
        
        cout << "\n【经验统计】" << endl;
        cout << "  获得 EXP: +" << stats.totalExpGained << endl;
        cout << "  消耗 EXP: -" << stats.totalExpSpent << endl;
        cout << "  净收益: " << (stats.totalExpGained - stats.totalExpSpent) << " EXP" << endl;
        
        cout << "\n【武器伤害统计】" << endl;
        if (stats.weaponStats.empty()) {
            cout << "  未使用武器" << endl;
        } else {
            for (const auto& ws : stats.weaponStats) {
                cout << "  " << ws.weaponName << ": " << ws.totalDamage << " 伤害 (" << ws.hits << " 次攻击)" << endl;
            }
        }
        
        cout << "\n";
        system("pause");
    }

public:
    AdventureSystem(vector<Monster> monsters, EquipmentSlot* equipment, int& exp)
        : allMonsters(monsters), playerEquipment(equipment), playerExp(exp),
          difficultyLevel(0), battlesUntilCampfire(3) {
        
        // 初始化随机数生成器
        rng.seed(static_cast<unsigned int>(time(nullptr)));
        
        // 计算玩家最大生命值
        if (equipment->equippedArmor) {
            playerMaxHp = equipment->equippedArmor->getMaxHp();
        } else {
            playerMaxHp = 100;  // 默认生命值
        }
        playerCurrentHp = playerMaxHp;
    }
    
    // 开始冒险
    void startAdventure(vector<Equipment*>& inventory) {
        system("cls");
        cout << "\n╔════════════════════════════════════╗" << endl;
        cout << "║       开始冒险！                   ║" << endl;
        cout << "╚════════════════════════════════════╝" << endl;
        
        // 检查怪物库
        if (allMonsters.empty()) {
            cout << "\n【错误】没有加载怪物数据！无法开始冒险。" << endl;
            system("pause");
            return;
        }
        
        cout << "\n[调试] 怪物库大小: " << allMonsters.size() << endl;
        
        if (!playerEquipment->equippedArmor) {
            cout << "\n【警告】未装备装甲！将使用默认生命值 100。" << endl;
            playerMaxHp = 100;
            playerCurrentHp = 100;
        } else {
            playerMaxHp = playerEquipment->equippedArmor->getMaxHp();
            playerCurrentHp = playerMaxHp;
            cout << "\n[调试] 装备装甲: " << playerEquipment->equippedArmor->getName() << endl;
        }
        
        if (playerEquipment->equippedWeapons.empty()) {
            cout << "\n【警告】未装备武器！无法造成伤害！" << endl;
        } else {
            cout << "\n[调试] 装备武器数量: " << playerEquipment->equippedWeapons.size() << endl;
            for (auto w : playerEquipment->equippedWeapons) {
                if (w) {
                    cout << "  - " << w->getName() << endl;
                }
            }
        }
        
        cout << "\n初始状态：" << endl;
        cout << "  HP: " << playerCurrentHp << "/" << playerMaxHp << endl;
        cout << "  EXP: " << playerExp << endl;
        cout << "  装备武器: " << playerEquipment->equippedWeapons.size() << " 件" << endl;
        
        system("pause");
        
        // 冒险主循环
        bool adventureContinues = true;
        while (adventureContinues) {
            // 战斗
            bool battleResult = singleBattle();
            
            if (!battleResult) {
                // 战斗失败
                cout << "\n【冒险失败】" << endl;
                cout << "你在战斗中被击败了..." << endl;
                system("pause");
                break;
            }
            
            // 战斗胜利后，提供修复服务
            if (playerCurrentHp < playerMaxHp) {
                repairService();
            }
            
            // 检查是否到达篝火
            if (battlesUntilCampfire <= 0) {
                bool continueCampfire = campfireRest(inventory);
                if (!continueCampfire) {
                    // 玩家选择返回基地
                    cout << "\n传送回基地..." << endl;
                    Sleep(1000);
                    adventureContinues = false;
                }
            }
        }
        
        // 显示冒险统计
        showAdventureStats();
        
        // 应用获得的EXP（结算）
        int netGain = stats.totalExpGained - stats.totalExpSpent;
        playerExp += netGain;
        
        cout << "\n【EXP结算】" << endl;
        if (netGain > 0) {
            cout << "  恭喜！本次冒险净收益 +" << netGain << " EXP" << endl;
        } else if (netGain < 0) {
            cout << "  本次冒险净亏损 " << netGain << " EXP" << endl;
        } else {
            cout << "  本次冒险收支平衡" << endl;
        }
        cout << "  当前总 EXP: " << playerExp << endl;
        system("pause");
    }
};

#endif

