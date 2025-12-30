/**
 * 文件名: main.cpp
 * 职责: 游戏主循环、界面显示 (View)、资源整合
 */

#include <iostream>
#include <iomanip>    // 用于 setw (表格对齐)
#include <vector>
#include <cstdlib>    // 用于 system("cls")
#include <limits>     // 用于清空输入缓冲区
#include <windows.h>
// --- 引入自定义头文件 ---
#include "GameCore.h"   // 核心类定义 (Equipment, Weapon, Armor)
#include "DataLoader.h" // 数据加载器
#include "SaveManager.h"
#include "Adventure.h"
#include "Shop.h"
using namespace std;

// ==========================================
// [界面模块] Display Class
// 负责所有 cout 输出，不包含游戏逻辑
// ==========================================
class Display {
public:
    // ANSI 颜色代码
    static const string COLOR_RESET;
    static const string COLOR_WHITE;
    static const string COLOR_GREEN;
    static const string COLOR_RED;
    static const string COLOR_YELLOW;
    
    // 根据稀有度获取颜色
    static string getRarityColor(Rarity r) {
        switch(r) {
            case BROKEN: return COLOR_WHITE;
            case STANDARD: return COLOR_GREEN;
            case MILITARY: return COLOR_RED;
            case LEGENDARY: return COLOR_YELLOW;
            default: return COLOR_WHITE;
        }
    }
    
    // 获取稀有度名称
    static string getRarityName(Rarity r) {
        switch(r) {
            case BROKEN: return "损坏";
            case STANDARD: return "普通";
            case MILITARY: return "军用";
            case LEGENDARY: return "传奇";
            default: return "未知";
        }
    }
    
    // 画分割线
    static void drawLine() {
        cout << "+-----------------------------------------------------------------------+" << endl;
    }

    // 显示单个装备详细信息
    static void showItem(Equipment* item, int index = -1) {
        if (!item) return;

        drawLine();
        // 显示序号（如果提供）
        if (index >= 0) {
            cout << "| [" << index << "] ";
        } else {
            cout << "| ";
        }
        
        // 带颜色的名称
        cout << getRarityColor(item->getRarity()) << item->getName() << COLOR_RESET;
        cout << " (" << getRarityName(item->getRarity()) << ")";
        cout << " | 等级: " << item->getLevel();
        cout << " | 评分: " << item->calculatePower() << endl;
        cout << "| 描述: " << item->getDescription() << endl;
        drawLine();
    }

    // 显示主菜单
    static void showMenu(int exp) {
        cout << "\n===============================" << endl;
        cout << "   钢之魂：核心重构 (Ver 1.0)   " << endl;
        cout << "===============================" << endl;
        cout << "当前 EXP: " << exp << endl;
        cout << "-------------------------------" << endl;
        cout << "[1] 查看机库 (Inventory)" << endl;
        cout << "[2] 装备管理 (Equip)" << endl;
        cout << "[3] 装备升级 (Upgrade)" << endl;
        cout << "[4] 出发冒险 (Adventure)" << endl;
        cout << "[5] 访问商店 (Shop)" << endl;
        cout << "[6] 手动存档 (Save)" << endl;
        cout << "[9] 测试：获得100 EXP" << endl;  // 测试用
        // cout << "[7] 装备合成实验 (Synthesis)" << endl;  // 暂时隐藏
        // cout << "[8] 查看怪物图鉴 (Bestiary)" << endl;  // 暂时隐藏
        cout << "[0] 退出系统 (Exit)" << endl;
        cout << "-------------------------------" << endl;
        cout << ">>> 请输入指令: ";
    }
    
    // 显示当前装备状态
    static void showEquipmentStatus(const EquipmentSlot& slot) {
        cout << "\n=== 当前装备状态 ===" << endl;
        
        if (slot.equippedArmor) {
            cout << "【装甲】: " << getRarityColor(slot.equippedArmor->getRarity()) 
                 << slot.equippedArmor->getName() << COLOR_RESET << endl;
            int dodgeRate = slot.getEffectiveDodgeRate();
            cout << "  血量: " << slot.equippedArmor->getMaxHp() 
                 << " | 承重: " << slot.equippedArmor->getCapacity()
                 << " | 闪避率: " << dodgeRate << "%" << endl;
        } else {
            cout << "【装甲】: 未装备" << endl;
        }
        
        cout << "\n【武器】(" << slot.equippedWeapons.size() << "件):" << endl;
        if (slot.equippedWeapons.empty()) {
            cout << "  未装备武器" << endl;
        } else {
            for (size_t i = 0; i < slot.equippedWeapons.size(); i++) {
                auto w = slot.equippedWeapons[i];
                cout << "  [" << i << "] " << getRarityColor(w->getRarity()) 
                     << w->getName() << COLOR_RESET
                     << " (攻击:" << w->getAtk() << " 重量:" << w->getWeight() << ")" << endl;
            }
        }
        
        int totalWeight = slot.getTotalWeight();
        int capacity = slot.equippedArmor ? slot.equippedArmor->getCapacity() : 0;
        cout << "\n总重量: " << totalWeight << " / " << capacity;
        if (capacity > 0 && totalWeight > capacity * 0.6) {
            cout << " [警告: 超重，闪避率归零！]";
        }
        cout << endl;
    }
};

// 定义颜色常量
const string Display::COLOR_RESET = "\033[0m";
const string Display::COLOR_WHITE = "\033[37m";
const string Display::COLOR_GREEN = "\033[32m";
const string Display::COLOR_RED = "\033[31m";
const string Display::COLOR_YELLOW = "\033[33m";

// ==========================================
// [商店状态管理] Shop State Functions
// ==========================================
void saveShopStates(int slotIndex, Shop& baseShop, Shop& campfireShop) {
    json shopJson;
    shopJson["base_shop"] = baseShop.toJson();
    shopJson["campfire_shop"] = campfireShop.toJson();
    
    string filename = "saves/shop_slot_" + to_string(slotIndex) + ".json";
    ofstream f(filename);
    if (f.is_open()) {
        f << shopJson.dump(4);
        cout << "[存档] 商店状态已保存。" << endl;
    }
}

void loadShopStates(int slotIndex, Shop& baseShop, Shop& campfireShop, const vector<Equipment*>& templates) {
    string filename = "saves/shop_slot_" + to_string(slotIndex) + ".json";
    ifstream f(filename);
    
    if (!f.is_open()) {
        // 如果没有商店存档，初始化为新商店
        baseShop.refresh();
        return;
    }
    
    try {
        json shopJson = json::parse(f);
        if (shopJson.contains("base_shop")) {
            baseShop.fromJson(shopJson["base_shop"], templates);
        }
        if (shopJson.contains("campfire_shop")) {
            campfireShop.fromJson(shopJson["campfire_shop"], templates);
        }
        cout << "[存档] 商店状态已恢复。" << endl;
    } catch (json::parse_error& e) {
        cout << "[警告] 商店存档解析失败，将重新初始化。" << endl;
        baseShop.refresh();
    }
}

// ==========================================
// [主程序] Main Function
// ==========================================
int main() {
    // 1. 环境初始化
    // Windows下强制使用UTF-8编码，防止中文乱码
    system("chcp 65001"); 
    system("cls"); // 清屏
    
    // 初始化游戏数据和存档槽位
    SaveManager::initGameData("gamedata.json");
    SaveManager::initializeSaveSlots();
    
    // 显示存档槽位信息
    SaveManager::showSaveSlots();
    
    int slot = 0;
    cout << "\n请选择存档槽位 (1-3): ";
    cin >> slot;

    string playerName = "User";
    int playerExp = 0;
    int equippedArmorId = -1;
    vector<int> equippedWeaponIds;
    
    // 尝试加载存档
    vector<Equipment*> inventory = SaveManager::loadSave(slot, playerName, playerExp, equippedArmorId, equippedWeaponIds);

    // 如果是空背包（说明是新存档），给个初始装备
    if (inventory.empty()) {
        cout << "请输入新驾驶员代号: ";
        cin >> playerName;
        
        // 给新玩家发放初始装备
        cout << "[系统] 正在发放新手礼包..." << endl;
        
        // 装甲
        Equipment* armor1 = SaveManager::getItemTemplate(201);
        if (armor1) inventory.push_back(armor1->clone(armor1->getName(), 1));
        
        Equipment* armor2 = SaveManager::getItemTemplate(203);
        if (armor2) inventory.push_back(armor2->clone(armor2->getName(), 1));
        
        // 武器
        Equipment* weapon1 = SaveManager::getItemTemplate(101);
        if (weapon1) inventory.push_back(weapon1->clone(weapon1->getName(), 1));
        
        Equipment* weapon2 = SaveManager::getItemTemplate(102);
        if (weapon2) inventory.push_back(weapon2->clone(weapon2->getName(), 1));
        
        Equipment* weapon3 = SaveManager::getItemTemplate(103);
        if (weapon3) inventory.push_back(weapon3->clone(weapon3->getName(), 1));
        
        cout << "[系统] 新手礼包发放完毕！获得 " << inventory.size() << " 件装备。" << endl;
    } else {
        cout << "[存档] 欢迎回来，" << playerName << "！" << endl;
    }
    Sleep(1000);
    system("cls");
    // 2. 数据加载 (Data Loading)
    // 加载怪物数据用于图鉴显示
    vector<Monster> monsters = DataLoader::loadMonsters("gamedata.json");
    
    // 初始化装备槽
    EquipmentSlot equipSlot;
    
    // 初始化基地商店和篝火商店
    vector<Equipment*> allEquipmentTemplates = SaveManager::getAllEquipmentTemplates();
    Shop baseShop(allEquipmentTemplates);
    Shop campfireShop(allEquipmentTemplates);
    
    // 加载商店状态
    loadShopStates(slot, baseShop, campfireShop, allEquipmentTemplates);
    
    // 恢复装备配置
    if (equippedArmorId != -1) {
        for (auto item : inventory) {
            if (item->getId() == equippedArmorId) {
                equipSlot.equippedArmor = dynamic_cast<Armor*>(item);
                cout << "[存档] 已恢复装备的装甲: " << item->getName() << endl;
                break;
            }
        }
    }
    
    for (int weaponId : equippedWeaponIds) {
        for (auto item : inventory) {
            if (item->getId() == weaponId) {
                Weapon* weapon = dynamic_cast<Weapon*>(item);
                if (weapon) {
                    equipSlot.equippedWeapons.push_back(weapon);
                    cout << "[存档] 已恢复装备的武器: " << item->getName() << endl;
                }
                break;
            }
        }
    }

    // 3. 游戏主循环 (Game Loop)
    bool isRunning = true;
    while (isRunning) {
        Display::showMenu(playerExp);
        
        int choice;
        // --- 输入检查 (Input Validation) ---
        if (!(cin >> choice)) {
            // 如果用户输入了字母而不是数字
            cout << "[错误] 无效输入，请输入数字！" << endl;
            cin.clear(); // 清除错误标志
            cin.ignore(numeric_limits<streamsize>::max(), '\n'); // 丢弃缓冲区内容
            continue;
        }
        system("cls");
        switch (choice) {
            case 0: // 退出
            {
                isRunning = false;
                // 保存游戏，包括装备配置
                vector<Equipment*> equippedWeaponsVec(equipSlot.equippedWeapons.begin(), equipSlot.equippedWeapons.end());
                SaveManager::saveGame(slot, playerName, inventory, playerExp, equipSlot.equippedArmor, equippedWeaponsVec);
                // 保存商店状态
                saveShopStates(slot, baseShop, campfireShop);
                SaveManager::cleanUp();
                cout << "正在将意识上传至云端..."<<endl;
                cout << "正在断开神经连接... 再见！" << endl;
                system("pause");
                break;
            }

            case 1: // 查看背包
                cout << "\n=== 当前机库库存 (" << inventory.size() << ") ===" << endl;
                for (size_t i = 0; i < inventory.size(); i++) {
                    // 多态调用：自动区分是 Weapon 还是 Armor 并显示对应描述
                    Display::showItem(inventory[i], i);
                }
                system("pause"); // 暂停，让用户看清楚
                break;

            case 2: // 装备管理
            {
                bool equipMenuRunning = true;
                while (equipMenuRunning) {
                    system("cls");
                    Display::showEquipmentStatus(equipSlot);
                    
                    cout << "\n=== 装备管理 ===" << endl;
                    cout << "[1] 装备装甲" << endl;
                    cout << "[2] 卸下装甲" << endl;
                    cout << "[3] 装备武器" << endl;
                    cout << "[4] 卸下武器" << endl;
                    cout << "[0] 返回主菜单" << endl;
                    cout << ">>> 请选择: ";
                    
                    int equipChoice;
                    if (!(cin >> equipChoice)) {
                        cin.clear();
                        cin.ignore(numeric_limits<streamsize>::max(), '\n');
                        continue;
                    }
                    
                    switch (equipChoice) {
                        case 0:
                            equipMenuRunning = false;
                            break;
                            
                        case 1: // 装备装甲
                        {
                            cout << "\n=== 可用装甲 ===" << endl;
                            vector<int> armorIndices;
                            for (size_t i = 0; i < inventory.size(); i++) {
                                Armor* armor = dynamic_cast<Armor*>(inventory[i]);
                                if (armor) {
                                    cout << "[" << armorIndices.size() << "] ";
                                    Display::showItem(armor);
                                    armorIndices.push_back(i);
                                }
                            }
                            
                            if (armorIndices.empty()) {
                                cout << "没有可装备的装甲！" << endl;
                                system("pause");
                                break;
                            }
                            
                            cout << "请选择要装备的装甲 (输入-1取消): ";
                            int armorChoice;
                            cin >> armorChoice;
                            
                            if (armorChoice >= 0 && armorChoice < (int)armorIndices.size()) {
                                equipSlot.equippedArmor = dynamic_cast<Armor*>(inventory[armorIndices[armorChoice]]);
                                cout << "已装备: " << equipSlot.equippedArmor->getName() << endl;
                            }
                            system("pause");
                            break;
                        }
                        
                        case 2: // 卸下装甲
                            if (equipSlot.equippedArmor) {
                                cout << "已卸下: " << equipSlot.equippedArmor->getName() << endl;
                                equipSlot.equippedArmor = nullptr;
                                equipSlot.equippedWeapons.clear(); // 卸下装甲时也卸下所有武器
                                cout << "所有武器也已卸下（没有装甲无法携带武器）" << endl;
                            } else {
                                cout << "当前未装备装甲！" << endl;
                            }
                            system("pause");
                            break;
                            
                        case 3: // 装备武器
                        {
                            if (!equipSlot.equippedArmor) {
                                cout << "请先装备装甲！" << endl;
                                system("pause");
                                break;
                            }
                            
                            cout << "\n=== 可用武器 ===" << endl;
                            vector<int> weaponIndices;
                            for (size_t i = 0; i < inventory.size(); i++) {
                                Weapon* weapon = dynamic_cast<Weapon*>(inventory[i]);
                                if (weapon) {
                                    // 检查是否已装备
                                    bool alreadyEquipped = false;
                                    for (auto w : equipSlot.equippedWeapons) {
                                        if (w == weapon) {
                                            alreadyEquipped = true;
                                            break;
                                        }
                                    }
                                    if (!alreadyEquipped) {
                                        cout << "[" << weaponIndices.size() << "] ";
                                        Display::showItem(weapon);
                                        weaponIndices.push_back(i);
                                    }
                                }
                            }
                            
                            if (weaponIndices.empty()) {
                                cout << "没有可装备的武器！" << endl;
                                system("pause");
                                break;
                            }
                            
                            cout << "请选择要装备的武器 (输入-1取消): ";
                            int weaponChoice;
                            cin >> weaponChoice;
                            
                            if (weaponChoice >= 0 && weaponChoice < (int)weaponIndices.size()) {
                                Weapon* selectedWeapon = dynamic_cast<Weapon*>(inventory[weaponIndices[weaponChoice]]);
                                int newWeight = equipSlot.getTotalWeight() + selectedWeapon->getWeight();
                                
                                if (newWeight > equipSlot.equippedArmor->getCapacity()) {
                                    cout << "超重！无法装备该武器。" << endl;
                                    cout << "当前重量: " << equipSlot.getTotalWeight() 
                                         << " + 武器重量: " << selectedWeapon->getWeight()
                                         << " > 承重上限: " << equipSlot.equippedArmor->getCapacity() << endl;
                                } else {
                                    equipSlot.equippedWeapons.push_back(selectedWeapon);
                                    cout << "已装备: " << selectedWeapon->getName() << endl;
                                    
                                    if (newWeight > equipSlot.equippedArmor->getCapacity() * 0.6) {
                                        cout << "[警告] 总重量超过承重60%，闪避率已归零！" << endl;
                                    }
                                }
                            }
                            system("pause");
                            break;
                        }
                        
                        case 4: // 卸下武器
                        {
                            if (equipSlot.equippedWeapons.empty()) {
                                cout << "当前未装备武器！" << endl;
                                system("pause");
                                break;
                            }
                            
                            cout << "\n=== 已装备的武器 ===" << endl;
                            for (size_t i = 0; i < equipSlot.equippedWeapons.size(); i++) {
                                cout << "[" << i << "] " << Display::getRarityColor(equipSlot.equippedWeapons[i]->getRarity())
                                     << equipSlot.equippedWeapons[i]->getName() << Display::COLOR_RESET << endl;
                            }
                            
                            cout << "请选择要卸下的武器 (输入-1取消): ";
                            int removeChoice;
                            cin >> removeChoice;
                            
                            if (removeChoice >= 0 && removeChoice < (int)equipSlot.equippedWeapons.size()) {
                                cout << "已卸下: " << equipSlot.equippedWeapons[removeChoice]->getName() << endl;
                                equipSlot.equippedWeapons.erase(equipSlot.equippedWeapons.begin() + removeChoice);
                            }
                            system("pause");
                            break;
                        }
                        
                        default:
                            cout << "无效选项！" << endl;
                            system("pause");
                            break;
                    }
                }
                break;
            }

            case 3: // 装备升级
            {
                cout << "\n=== 装备升级系统 ===" << endl;
                cout << "当前 EXP: " << playerExp << endl;
                cout << "\n可升级的装备：" << endl;
                
                // 创建统一的装备列表
                vector<Equipment*> upgradeableEquipment;
                
                for (size_t i = 0; i < inventory.size(); i++) {
                    upgradeableEquipment.push_back(inventory[i]);
                }
                
                if (upgradeableEquipment.empty()) {
                    cout << "没有装备！" << endl;
                    system("pause");
                    break;
                }
                
                // 统一显示所有装备
                for (size_t i = 0; i < upgradeableEquipment.size(); i++) {
                    Equipment* equip = upgradeableEquipment[i];
                    
                    // 判断是武器还是装甲
                    Weapon* weapon = dynamic_cast<Weapon*>(equip);
                    Armor* armor = dynamic_cast<Armor*>(equip);
                    
                    cout << "[" << i << "] ";
                    cout << Display::getRarityColor(equip->getRarity()) 
                         << equip->getName() << Display::COLOR_RESET;
                    
                    if (weapon) {
                        cout << " [武器]";
                    } else if (armor) {
                        cout << " [装甲]";
                    }
                    
                    cout << " | 等级: " << equip->getLevel() << "/3";
                    
                    if (weapon && weapon->canLevelUp()) {
                        int cost = weapon->getUpgradeCost();
                        cout << " | 升级消耗: " << cost << " EXP";
                        if (playerExp >= cost) {
                            cout << " [可升级]";
                        } else {
                            cout << " [EXP不足]";
                        }
                    } else if (armor && armor->canLevelUp()) {
                        int cost = armor->getUpgradeCost();
                        cout << " | 升级消耗: " << cost << " EXP";
                        if (playerExp >= cost) {
                            cout << " [可升级]";
                        } else {
                            cout << " [EXP不足]";
                        }
                    } else {
                        cout << " [已满级]";
                    }
                    cout << endl;
                    
                    // 显示属性
                    if (weapon) {
                        cout << "    当前: 攻击" << weapon->getAtk() 
                             << " 暴击" << weapon->getCritRate() << "% "
                             << " 速度" << weapon->getAtkSpeed() << endl;
                        
                        if (weapon->canLevelUp()) {
                            int nextAtk = static_cast<int>(weapon->getBaseAtk() * (1.0 + 0.2 * weapon->getLevel()));
                            int nextCrit = static_cast<int>(weapon->getBaseCritRate() * (1.0 + 0.2 * weapon->getLevel()));
                            if (nextCrit > 100) nextCrit = 100;
                            int nextSpeed = static_cast<int>(weapon->getBaseAtkSpeed() / (1.0 + 0.2 * weapon->getLevel()));
                            if (nextSpeed < 1) nextSpeed = 1;
                            
                            cout << "    升级后: 攻击" << nextAtk 
                                 << " 暴击" << nextCrit << "% "
                                 << " 速度" << nextSpeed << endl;
                        }
                    } else if (armor) {
                        cout << "    当前: 血量" << armor->getMaxHp() 
                             << " 闪避" << armor->getDodgeRate() << "% "
                             << " 承重" << armor->getCapacity() << endl;
                        
                        if (armor->canLevelUp()) {
                            int nextHp = static_cast<int>(armor->getBaseMaxHp() * (1.0 + 0.2 * armor->getLevel()));
                            int nextDodge = static_cast<int>(armor->getBaseDodgeRate() * (1.0 + 0.2 * armor->getLevel()));
                            if (nextDodge > 100) nextDodge = 100;
                            int nextCap = static_cast<int>(armor->getBaseCapacity() * (1.0 + 0.2 * armor->getLevel()));
                            
                            cout << "    升级后: 血量" << nextHp 
                                 << " 闪避" << nextDodge << "% "
                                 << " 承重" << nextCap << endl;
                        }
                    }
                    cout << endl;
                }
                
                cout << "请选择要升级的装备编号 (输入-1取消): ";
                int upgradeChoice;
                cin >> upgradeChoice;
                
                if (upgradeChoice >= 0 && upgradeChoice < (int)upgradeableEquipment.size()) {
                    Equipment* selectedEquip = upgradeableEquipment[upgradeChoice];
                    Weapon* selectedWeapon = dynamic_cast<Weapon*>(selectedEquip);
                    Armor* selectedArmor = dynamic_cast<Armor*>(selectedEquip);
                    
                    if (selectedWeapon) {
                        // 升级武器
                        if (!selectedWeapon->canLevelUp()) {
                            cout << "该武器已达到最高等级！" << endl;
                        } else {
                            int cost = selectedWeapon->getUpgradeCost();
                            if (playerExp < cost) {
                                cout << "EXP不足！需要 " << cost << " EXP，当前只有 " << playerExp << " EXP。" << endl;
                            } else {
                                playerExp -= cost;
                                selectedWeapon->levelUp();
                                cout << "\n升级成功！" << endl;
                                cout << Display::getRarityColor(selectedWeapon->getRarity()) 
                                     << selectedWeapon->getName() << Display::COLOR_RESET 
                                     << " 已升级到 Lv." << selectedWeapon->getLevel() << "！" << endl;
                                cout << "消耗 " << cost << " EXP，剩余 " << playerExp << " EXP。" << endl;
                                
                                // 显示新属性
                                cout << "\n新属性：" << endl;
                                cout << "  攻击力: " << selectedWeapon->getAtk() << endl;
                                cout << "  暴击率: " << selectedWeapon->getCritRate() << "%" << endl;
                                cout << "  攻击速度: " << selectedWeapon->getAtkSpeed() << " 回合/次" << endl;
                                cout << "  战斗力: " << selectedWeapon->calculatePower() << endl;
                            }
                        }
                    } else if (selectedArmor) {
                        // 升级装甲
                        if (!selectedArmor->canLevelUp()) {
                            cout << "该装甲已达到最高等级！" << endl;
                        } else {
                            int cost = selectedArmor->getUpgradeCost();
                            if (playerExp < cost) {
                                cout << "EXP不足！需要 " << cost << " EXP，当前只有 " << playerExp << " EXP。" << endl;
                            } else {
                                playerExp -= cost;
                                selectedArmor->levelUp();
                                cout << "\n升级成功！" << endl;
                                cout << Display::getRarityColor(selectedArmor->getRarity()) 
                                     << selectedArmor->getName() << Display::COLOR_RESET 
                                     << " 已升级到 Lv." << selectedArmor->getLevel() << "！" << endl;
                                cout << "消耗 " << cost << " EXP，剩余 " << playerExp << " EXP。" << endl;
                                
                                // 显示新属性
                                cout << "\n新属性：" << endl;
                                cout << "  血量上限: " << selectedArmor->getMaxHp() << endl;
                                cout << "  闪避率: " << selectedArmor->getDodgeRate() << "%" << endl;
                                cout << "  承重量: " << selectedArmor->getCapacity() << endl;
                                cout << "  战斗力: " << selectedArmor->calculatePower() << endl;
                            }
                        }
                    }
                }
                system("pause");
                break;
            }

            case 4: // 出发冒险
            {
                cout << "\n=== 冒险准备 ===" << endl;
                
                // 检查是否装备了装甲和武器
                if (!equipSlot.equippedArmor) {
                    cout << "【警告】未装备装甲！" << endl;
                    cout << "建议先装备装甲再出发冒险。" << endl;
                    cout << "\n是否继续？(1=是, 0=否): ";
                    int confirm;
                    cin >> confirm;
                    if (confirm != 1) {
                        break;
                    }
                }
                
                if (equipSlot.equippedWeapons.empty()) {
                    cout << "【警告】未装备武器！" << endl;
                    cout << "没有武器将无法造成伤害！" << endl;
                    cout << "\n是否继续？(1=是, 0=否): ";
                    int confirm;
                    cin >> confirm;
                    if (confirm != 1) {
                        break;
                    }
                }
                
                // 开始冒险
                AdventureSystem adventure(monsters, &equipSlot, playerExp, &campfireShop);
                adventure.startAdventure(inventory);
                
                // 冒险结束后，标记基地商店需要刷新
                baseShop.markNeedsRefresh();
                break;
            }
            
            case 5: // 访问商店
            {
                system("cls");
                cout << "\n=== 基地商店 ===" << endl;
                
                // 如果需要刷新，刷新商店
                if (baseShop.isNeedsRefresh()) {
                    baseShop.refresh();
                }
                
                while (true) {
                    cout << "\n";
                    baseShop.display();
                    cout << "\n当前 EXP: " << playerExp << endl;
                    cout << "\n[1-3] 购买对应商品 | [0] 返回主菜单" << endl;
                    cout << ">>> 请选择: ";
                    
                    int shopChoice;
                    cin >> shopChoice;
                    
                    if (shopChoice == 0) {
                        break;
                    } else if (shopChoice >= 1 && shopChoice <= 3) {
                        if (baseShop.buyItem(shopChoice - 1, playerExp, inventory)) {
                            cout << "\n[提示] 装备已添加到背包！" << endl;
                        }
                        system("pause");
                        system("cls");
                        cout << "\n=== 基地商店 ===" << endl;
                    } else {
                        cout << "无效选项！" << endl;
                    }
                }
                break;
            }
            
            case 6: // 手动存档
            {
                cout << "\n=== 手动存档 ===" << endl;
                cout << "正在保存游戏进度..." << endl;
                vector<Equipment*> equippedWeaponsVec(equipSlot.equippedWeapons.begin(), equipSlot.equippedWeapons.end());
                SaveManager::saveGame(slot, playerName, inventory, playerExp, equipSlot.equippedArmor, equippedWeaponsVec);
                // 保存商店状态
                saveShopStates(slot, baseShop, campfireShop);
                cout << "存档完成！" << endl;
                system("pause");
                break;
            }
            
            case 9: // 测试：获得EXP
                playerExp += 100;
                cout << "获得 100 EXP！当前 EXP: " << playerExp << endl;
                system("pause");
                break;

            // case 6: // 合成测试 (已隐藏)
            // case 7: // 查看怪物 (已隐藏)

            default:
                cout << "未知指令，请重新输入。" << endl;
                break;
        }
        
        // 每次操作完清屏一次，保持界面整洁 (可选)
        system("cls"); 
    }

    // 4. 内存清理 (Resource Management)
    // 释放所有 new 出来的 Equipment 对象，防止内存泄漏
    for (auto item : inventory) {
        delete item;
    }
    inventory.clear();

    return 0;
}