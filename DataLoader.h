// DataLoader.h (或者直接写在 main.cpp 上方)
#ifndef DATALOADER_H
#define DATALOADER_H

#include <fstream>
#include <iostream>
#include <vector>
#include "json.hpp"
#include "GameCore.h" // 引用之前的核心类

using json = nlohmann::json;
using namespace std;

// 简单的怪物类结构 (示例)
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

class DataLoader {
private:
    // 辅助函数：将字符串转换为枚举
    static Rarity stringToRarity(string s) {
        if (s == "BROKEN") return BROKEN;
        if (s == "STANDARD") return STANDARD;
        if (s == "MILITARY") return MILITARY;
        if (s == "LEGENDARY") return LEGENDARY;
        return BROKEN; // 默认值
    }

public:
    // 加载装备
    static vector<Equipment*> loadEquipment(const string& filename) {
        vector<Equipment*> result;
        ifstream f(filename);

        if (!f.is_open()) {
            cerr << "[错误] 无法打开文件: " << filename << endl;
            return result; // 返回空列表
        }

        try {
            json data = json::parse(f); // 解析整个文件

            for (auto& item : data) {
                int id = item.value("id", 0);
                string type = item["type"];
                string name = item["name"];
                Rarity rarity = stringToRarity(item["rarity"]);
                int level = item["level"];
                string faction = item["faction"];

                // 工厂模式的核心：根据 type 创建不同的子类
                if (type == "weapon") {
                    int atk = item["atk"];
                    int critRate = item.value("crit_rate", 0);  // 已经是*100后的值
                    int atkSpeed = item.value("atk_speed", 1);
                    int weight = item.value("weight", 1);
                    result.push_back(new Weapon(id, name, rarity, level, faction, atk, critRate, atkSpeed, weight));
                } 
                else if (type == "armor") {
                    int hp = item["hp"];
                    int dodgeRate = item.value("dodge_rate", 0);  // 已经是*100后的值
                    int capacity = item.value("capacity", 10);
                    result.push_back(new Armor(id, name, rarity, level, faction, hp, dodgeRate, capacity));
                }
            }
            cout << "[系统] 成功加载 " << result.size() << " 件装备。" << endl;
        } 
        catch (json::parse_error& e) {
            cerr << "[JSON错误] 解析失败: " << e.what() << endl;
        }

        return result;
    }

    // 加载怪物
    static vector<Monster> loadMonsters(const string& filename) {
        vector<Monster> result;
        ifstream f(filename);
        
        if (!f.is_open()) {
            cerr << "[错误] 无法打开文件: " << filename << endl;
            return result;
        }

        try {
            json data = json::parse(f);
            
            // 检查是否有 "monsters" 键（gamedata.json 格式）
            json monsterArray;
            if (data.contains("monsters")) {
                monsterArray = data["monsters"];
            } else {
                // 如果没有，假设整个文件就是怪物数组（enemy.json 格式）
                monsterArray = data;
            }
            
            for (auto& item : monsterArray) {
                Monster m;
                m.id = item["id"];
                m.name = item["name"];
                m.hp = item["hp"];
                m.atk = item["atk"];
                m.exp = item["exp"];
                result.push_back(m);
            }
            cout << "[系统] 成功加载 " << result.size() << " 个怪物数据。" << endl;
        }
        catch (json::parse_error& e) {
            cerr << "[JSON错误] " << e.what() << endl;
        }
        return result;
    }
};

#endif // DATALOADER_H