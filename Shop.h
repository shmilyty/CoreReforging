/**
 * 文件名: Shop.h
 * 职责: 商店系统 - 装备购买、商品刷新
 */

#ifndef SHOP_H
#define SHOP_H

#include <iostream>
#include <vector>
#include <random>
#include <ctime>
#include <iomanip>
#include "GameCore.h"

using namespace std;

// 商店物品结构
struct ShopItem {
    Equipment* equipment;
    int price;
    
    ShopItem(Equipment* eq, int p) : equipment(eq), price(p) {}
};

// 商店类
class Shop {
private:
    vector<ShopItem> items;           // 当前商店物品
    vector<Equipment*> allEquipments; // 所有可用装备模板
    mt19937 rng;
    bool needsRefresh;                // 是否需要刷新
    
    // 获取稀有度对应的颜色代码
    string getRarityColor(Rarity r) const {
        switch(r) {
            case Rarity::BROKEN: return "\033[37m";    // 白色
            case Rarity::STANDARD: return "\033[32m";  // 绿色
            case Rarity::MILITARY: return "\033[31m";  // 红色
            case Rarity::LEGENDARY: return "\033[33m"; // 黄色
            default: return "\033[37m";
        }
    }
    
    // 计算装备价格
    int calculatePrice(Equipment* eq) const {
        return 500 * (static_cast<int>(eq->getRarity()) + 1);
    }
    
public:
    Shop(const vector<Equipment*>& equipmentTemplates) 
        : allEquipments(equipmentTemplates), needsRefresh(true) {
        rng.seed(static_cast<unsigned int>(time(nullptr)));
    }
    
    // 刷新商店（随机3件装备）
    void refresh() {
        // 清空旧物品
        for (auto& item : items) {
            delete item.equipment;
        }
        items.clear();
        
        if (allEquipments.empty()) {
            cout << "[错误] 没有可用的装备模板！" << endl;
            return;
        }
        
        // 随机选择3件装备
        uniform_int_distribution<int> dist(0, allEquipments.size() - 1);
        for (int i = 0; i < 3; i++) {
            int idx = dist(rng);
            Equipment* template_eq = allEquipments[idx];
            Equipment* eq = template_eq->clone(template_eq->getName(), 1);
            int price = calculatePrice(eq);
            items.push_back(ShopItem(eq, price));
        }
        
        needsRefresh = false;
        cout << "[系统] 商店已刷新！" << endl;
    }
    
    // 显示商店
    void display() const {
        cout << "\n========== 商店 ==========" << endl;
        if (items.empty()) {
            cout << "商店暂无商品。" << endl;
            return;
        }
        
        for (size_t i = 0; i < items.size(); i++) {
            Equipment* eq = items[i].equipment;
            string color = getRarityColor(eq->getRarity());
            
            cout << "[" << (i + 1) << "] " << color << eq->getName() << "\033[0m";
            
            // 显示装备类型和属性
            Weapon* w = dynamic_cast<Weapon*>(eq);
            Armor* a = dynamic_cast<Armor*>(eq);
            
            if (w) {
                cout << " (武器)";
                cout << " | 攻击:" << w->getAtk();
                cout << " | 暴击率:" << w->getCritRate() << "%";
                cout << " | 攻速:" << w->getAtkSpeed();
                cout << " | 重量:" << w->getWeight();
            } else if (a) {
                cout << " (装甲)";
                cout << " | HP:" << a->getMaxHp();
                cout << " | 闪避率:" << a->getDodgeRate() << "%";
                cout << " | 承重:" << a->getCapacity();
            }
            
            cout << " | 价格: " << items[i].price << " EXP" << endl;
        }
        cout << "==========================" << endl;
    }
    
    // 购买物品
    bool buyItem(int index, int& playerExp, vector<Equipment*>& inventory) {
        if (index < 0 || index >= static_cast<int>(items.size())) {
            cout << "[错误] 无效的选择！" << endl;
            return false;
        }
        
        ShopItem& item = items[index];
        
        // 检查经验值是否足够
        if (playerExp < item.price) {
            cout << "[错误] EXP 不足！需要 " << item.price << " EXP，当前只有 " << playerExp << " EXP。" << endl;
            return false;
        }
        
        // 扣除经验值
        playerExp -= item.price;
        
        // 添加到背包
        Equipment* purchased = item.equipment->clone(item.equipment->getName(), item.equipment->getLevel());
        inventory.push_back(purchased);
        
        cout << "[成功] 购买了 " << item.equipment->getName() << "！" << endl;
        cout << "[系统] 剩余 EXP: " << playerExp << endl;
        
        // 从商店移除该物品
        delete item.equipment;
        items.erase(items.begin() + index);
        
        return true;
    }
    
    // 标记需要刷新
    void markNeedsRefresh() {
        needsRefresh = true;
    }
    
    // 检查是否需要刷新
    bool isNeedsRefresh() const {
        return needsRefresh;
    }
    
    // 获取商店物品数量
    int getItemCount() const {
        return items.size();
    }
    
    // 序列化商店状态（用于存档）
    json toJson() const {
        json j;
        j["needs_refresh"] = needsRefresh;
        
        json itemsArray = json::array();
        for (const auto& item : items) {
            json itemJson;
            itemJson["equipment_id"] = item.equipment->getId();
            itemJson["equipment_level"] = item.equipment->getLevel();
            itemJson["price"] = item.price;
            itemsArray.push_back(itemJson);
        }
        j["items"] = itemsArray;
        
        return j;
    }
    
    // 从 JSON 加载商店状态
    void fromJson(const json& j, const vector<Equipment*>& equipmentTemplates) {
        // 清空旧物品
        for (auto& item : items) {
            delete item.equipment;
        }
        items.clear();
        
        needsRefresh = j.value("needs_refresh", true);
        
        if (j.contains("items")) {
            for (const auto& itemJson : j["items"]) {
                int equipId = itemJson["equipment_id"];
                int equipLevel = itemJson.value("equipment_level", 1);
                int price = itemJson["price"];
                
                // 查找对应的装备模板
                Equipment* template_eq = nullptr;
                for (auto eq : equipmentTemplates) {
                    if (eq->getId() == equipId) {
                        template_eq = eq;
                        break;
                    }
                }
                
                if (template_eq) {
                    Equipment* eq = template_eq->clone(template_eq->getName(), equipLevel);
                    items.push_back(ShopItem(eq, price));
                }
            }
        }
    }
    
    // 析构函数
    ~Shop() {
        for (auto& item : items) {
            delete item.equipment;
        }
    }
};

#endif // SHOP_H

