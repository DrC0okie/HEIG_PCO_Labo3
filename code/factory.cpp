/**
 * @file factory.cpp
 * @brief Implementation of the Factory class
 * @author Aubry Mangold <aubry.mangold@heig-vd.ch>
 * @author Timothée Van Hove <timothee.vanhove@heig-vd.ch>
 * @date 2023-10-18
 */

#include "factory.h"
#include <pcosynchro/pcothread.h>
#include <iostream>
#include <mutex>
#include "costs.h"
#include "extractor.h"
#include "wholesale.h"

WindowInterface* Factory::interface = nullptr;


Factory::Factory(int uniqueId, int fund, ItemType builtItem,
                 std::vector<ItemType> resourcesNeeded)
    : Seller(fund, uniqueId),
      resourcesNeeded(resourcesNeeded),
      itemBuilt(builtItem),
      nbBuild(0) {
    assert(builtItem == ItemType::Chip || builtItem == ItemType::Plastic ||
           builtItem == ItemType::Robot);

    interface->updateFund(uniqueId, fund);
    interface->consoleAppendText(uniqueId, "Factory created");
}

void Factory::setWholesalers(std::vector<Wholesale*> wholesalers) {
    Factory::wholesalers = wholesalers;

    for (Seller* seller : wholesalers) {
        interface->setLink(uniqueId, seller->getUniqueId());
    }
}

ItemType Factory::getItemBuilt() {
    return itemBuilt;
}

int Factory::getMaterialCost() {
    return getCostPerUnit(itemBuilt);
}

bool Factory::verifyResources() {
    for (auto item : resourcesNeeded) {
        if (stocks[item] == 0) {
            return false;
        }
    }

    return true;
}

void Factory::buildItem() {
    int salary = getEmployeeSalary(getEmployeeThatProduces(itemBuilt));

    transactionMutex.lock();
    // If we have enough money to pay salary
    if (money < salary) {
        transactionMutex.unlock();
        return;
    }

    // produce item
    for (ItemType item : resourcesNeeded) {
        stocks[item]--;
    }

    // Pay salary
    money -= salary;
    transactionMutex.unlock();

    // Temps simulant l'assemblage d'un objet.
    PcoThread::usleep((rand() % 100) * 100000);

    // Increment number of payed employee
    nbBuild++;

    // update item stock
    transactionMutex.lock();
    stocks[itemBuilt]++;
    transactionMutex.unlock();

    // Update interface
    interface->consoleAppendText(uniqueId, "Factory have build a new object");
}

void Factory::orderResources() {
    transactionMutex.lock();
    // Prioritizing resources the factory has the least of.
    ItemType resourceToBuy = std::min_element(stocks.cbegin(), stocks.cend(),
                                              [](const auto& l, const auto& r) {
                                                  return l.second < r.second;
                                              })->first;

    // Iterate over available wholesalers
    for (Wholesale* ws : wholesalers) {
        if (ws->getItemsForSale().contains(resourceToBuy)) {
            int cost = getCostPerUnit(resourceToBuy);
            if (cost > money)
                break;
            cost = ws->trade(resourceToBuy, 1);
            if (cost == 0)
                continue;  // Trade did not work. Look at another wholeseller.
            stocks[resourceToBuy]++;
            money -= cost;
            break;
        }
    }
    transactionMutex.unlock();

    // Temps de pause pour éviter trop de demande
    PcoThread::usleep(10 * 100000);
}

void Factory::run() {
    if (wholesalers.empty()) {
        std::cerr << "You have to give to factories wholesalers to sales their "
                     "resources"
                  << std::endl;
        return;
    }
    interface->consoleAppendText(uniqueId, "[START] Factory routine");

    while (!PcoThread::thisThread()->stopRequested()) {
        if (verifyResources()) {
            buildItem();
        } else {
            orderResources();
        }
        interface->updateFund(uniqueId, money);
        interface->updateStock(uniqueId, &stocks);
    }
    interface->consoleAppendText(uniqueId, "[STOP] Factory routine");
}

std::map<ItemType, int> Factory::getItemsForSale() {
    return std::map<ItemType, int>({
        {itemBuilt, stocks[itemBuilt]}
    });
}

int Factory::trade(ItemType it, int qty) {
    transactionMutex.lock();
    if (qty <= 0 || it != itemBuilt || stocks[itemBuilt] < qty) {
        transactionMutex.unlock();
        return 0;
    }

    int cost = getCostPerUnit(it) * qty;

    money += cost;
    stocks[it] -= qty;
    transactionMutex.unlock();

    return cost;
}

int Factory::getAmountPaidToWorkers() {
    return Factory::nbBuild *
           getEmployeeSalary(getEmployeeThatProduces(itemBuilt));
}

void Factory::setInterface(WindowInterface* windowInterface) {
    interface = windowInterface;
}

PlasticFactory::PlasticFactory(int uniqueId, int fund)
    : Factory::Factory(uniqueId, fund, ItemType::Plastic, {ItemType::Petrol}) {}

ChipFactory::ChipFactory(int uniqueId, int fund)
    : Factory::Factory(uniqueId, fund, ItemType::Chip,
                       {ItemType::Sand, ItemType::Copper}) {}

RobotFactory::RobotFactory(int uniqueId, int fund)
    : Factory::Factory(uniqueId, fund, ItemType::Robot,
                       {ItemType::Chip, ItemType::Plastic}) {}
