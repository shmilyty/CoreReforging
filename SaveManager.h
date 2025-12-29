// SaveManager.h
#ifndef SAVE_MANAGER_H
#define SAVE_MANAGER_H

#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <string>
#include "json.hpp" // 确保有 nlohmann/json
#include "GameCore.h"

using json = nlohmann::json;
using namespace std;

class SaveManager {
private:
    // [静态数据库]：ID -> 装备模板的映射
    // 就像一个图书馆，存着所有装备的原始样本
    static map<int, Equipment*> itemLibrary;
    static vector<Monster> monsterLibrary;

public:
    // 获取装备库的访问方法
    static Equipment* getItemTemplate(int id) {
        if (itemLibrary.count(id)) {
            return itemLibrary[id];
        }
        return nullptr;
    }
    // 1. 初始化：加载所有游戏数据 (由队友设计的)
    static void initGameData(const string& dbFile) {
        ifstream f(dbFile);
        if (!f.is_open()) {
            cout << "[错误] 找不到游戏数据文件: " << dbFile << endl;
            return;
        }

        json j = json::parse(f);

        // 加载装备模板
        for (auto& item : j["equipments"]) {
            int id = item["id"];
            string type = item["type"];
            string name = item["name"];
            string fac = item["faction"];
            Rarity rar = STANDARD;
            if (item.contains("rarity")) {
                string rarStr = item["rarity"];
                if (rarStr == "BROKEN") rar = BROKEN;
                else if (rarStr == "STANDARD") rar = STANDARD;
                else if (rarStr == "MILITARY") rar = MILITARY;
                else if (rarStr == "LEGENDARY") rar = LEGENDARY;
            }
            
            // 创建模板对象存入 map，作为原型
            if (type == "weapon") {
                int atk = item["atk"];
                int critRate = item.value("crit_rate", 0);  // 已经是*100后的值
                int atkSpeed = item.value("atk_speed", 1);
                int weight = item.value("weight", 1);
                itemLibrary[id] = new Weapon(id, name, rar, 0, fac, atk, critRate, atkSpeed, weight);
            } else if (type == "armor") {
                int hp = item["hp"];
                int dodgeRate = item.value("dodge_rate", 0);  // 已经是*100后的值
                int capacity = item.value("capacity", 10);
                itemLibrary[id] = new Armor(id, name, rar, 0, fac, hp, dodgeRate, capacity);
            }
        }
        cout << "[系统] 游戏数据库加载完毕，收录装备数: " << itemLibrary.size() << endl;
    }

    // 2. 保存存档 (Serialization)
    static void saveGame(int slotIndex, const string& playerName, const vector<Equipment*>& inventory, int playerExp = 0) {
        json saveJson;
        saveJson["player_name"] = playerName;
        saveJson["exp"] = playerExp;
        
        // 序列化背包
        json invArray = json::array();
        for (auto item : inventory) {
            json itemJson;
            itemJson["tid"] = item->getId(); // 只存ID
            itemJson["lv"] = item->getLevel(); // 存当前等级
            itemJson["rar"] = static_cast<int>(item->getRarity()); // 存当前稀有度
            invArray.push_back(itemJson);
        }
        saveJson["inventory"] = invArray;

        // 写入文件到 saves 文件夹
        string filename = "saves/save_slot_" + to_string(slotIndex) + ".json";
        ofstream f(filename);
        f << saveJson.dump(4); // 缩进4空格，美观
        cout << "[存档] 游戏已保存至槽位 " << slotIndex << endl;
    }

    // 3. 加载存档 (Deserialization)
    static vector<Equipment*> loadSave(int slotIndex, string& playerName, int& playerExp) {
        vector<Equipment*> result;
        string filename = "saves/save_slot_" + to_string(slotIndex) + ".json";
        ifstream f(filename);

        if (!f.is_open()) {
            cout << "[提示] 存档槽 " << slotIndex << " 为空，将开始新游戏。" << endl;
            playerExp = 0;
            return result; // 返回空背包
        }

        json j = json::parse(f);
        playerName = j["player_name"];
        playerExp = j.value("exp", 0);  // 读取exp，默认为0

        for (auto& itemJson : j["inventory"]) {
            int tid = itemJson["tid"];
            int lv = itemJson["lv"];
            int rar = itemJson["rar"];

            // [关键步骤] 查表 -> 克隆 -> 恢复状态
            if (itemLibrary.count(tid)) {
                Equipment* prototype = itemLibrary[tid];
                // 我们调用 clone，并传入存档里的等级
                Equipment* newItem = prototype->clone(prototype->getName(), lv);
                // 这里可能需要扩展 setRarity 方法来恢复稀有度
                result.push_back(newItem);
            }
        }
        cout << "[存档] 读取存档槽 " << slotIndex << " 成功！" << endl;
        return result;
    }
    
    // 4. 初始化所有存档槽位
    static void initializeSaveSlots() {
        for (int i = 1; i <= 3; i++) {
            string filename = "saves/save_slot_" + to_string(i) + ".json";
            ifstream checkFile(filename);
            
            // 如果存档文件不存在，创建空存档
            if (!checkFile.is_open()) {
                json emptySlot;
                emptySlot["player_name"] = "";
                emptySlot["exp"] = 0;
                emptySlot["inventory"] = json::array();
                
                ofstream f(filename);
                f << emptySlot.dump(4);
                f.close();
            } else {
                checkFile.close();
            }
        }
    }
    
    // 5. 检查存档槽位是否为空
    static bool isSlotEmpty(int slotIndex) {
        string filename = "saves/save_slot_" + to_string(slotIndex) + ".json";
        ifstream f(filename);
        
        if (!f.is_open()) {
            return true;
        }
        
        json j = json::parse(f);
        f.close();
        
        // 检查玩家名字是否为空
        string playerName = j.value("player_name", "");
        return playerName.empty();
    }
    
    // 6. 显示所有存档槽位信息
    static void showSaveSlots() {
        cout << "\n=== 存档槽位 ===" << endl;
        for (int i = 1; i <= 3; i++) {
            cout << "[" << i << "] 存档 " << i << " - ";
            
            if (isSlotEmpty(i)) {
                cout << "空槽位" << endl;
            } else {
                string filename = "saves/save_slot_" + to_string(i) + ".json";
                ifstream f(filename);
                json j = json::parse(f);
                f.close();
                
                string playerName = j.value("player_name", "未知");
                int exp = j.value("exp", 0);
                int itemCount = j["inventory"].size();
                
                cout << playerName << " (EXP: " << exp << ", 装备: " << itemCount << "件)" << endl;
            }
        }
        cout << "===================" << endl;
    }
    
    // 静态成员变量需要在类外初始化
    static void cleanUp() {
        for(auto& pair : itemLibrary) delete pair.second;
        itemLibrary.clear();
    }
};

// 静态成员初始化
map<int, Equipment*> SaveManager::itemLibrary;
vector<Monster> SaveManager::monsterLibrary;

#endif