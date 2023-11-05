# HEIG_PCO_Labo3

**Authors**: Aubry Mangold, Timothée Van Hove

**Date**: 07.11.2023

## Introduction

In this lab, we were given the code base of a simulation program simulating production and sales between multiple entities. The job consists in implementing the logic governing the interactions between the various  players, while paying attention to the critical sections. The development of the resource management simulation has necessitated the implementation of several methods across the `extractors`, `wholesalers`, and `factories` classes to emulate the transactions. Our methodology emphasizes thread safety and transactional integrity ensuring that the simulation behaves predictably under concurrent conditions.



## The `trade` methods

The `trade` methods have been implemented in the `Extractor`, `Wholesale` and `Factory` classes. The implementation is globally the same in each of those classes, and designed to handle the transaction of a specific resource. It first acquires a lock to ensure thread safety, preventing simultaneous access that could lead to inconsistent states. Upon locking, the method verifies the validity of the trade, checking the requested quantity against the inventory and matching the item type to the extractor's resource. If the trade is invalid, it releases the lock and returns zero to indicate failure. For successful trades, it calculates the cost, adjusts the inventory and funds accordingly, and then releases the lock. This method provides a thread-safe means for extractors to participate in the market effectively.

trade method in the `Extractor` class:

````c++
int Extractor::trade(ItemType it, int qty) {
    transactionMutex.lock();
    // Check trade validity
    if ( qty <= 0 || it != resourceExtracted || stocks[it] < qty) {
        transactionMutex.unlock();
        return 0;
    }

    int cost = qty * getMaterialCost();
    money += cost;

    stocks[it] -= qty;
    transactionMutex.unlock();

    return cost;
}
````



## Wholesalers

`buyResources` method:

Implemented to get resources from the extractors, the `buyResources` method is designed to allocate funds for purchasing resources necessary for various wholesalers. The implementation is straightforward : We lock the transaction section, and if the wholesaler has enough money, it buys from a random extractor by decrementing the money and incrementing the stock.



````c++
void Wholesale::buyResources() {
    auto s = Seller::chooseRandomSeller(sellers);
    auto m = s->getItemsForSale();
    auto i = Seller::chooseRandomItem(m);

    if (i == ItemType::Nothing) {
        /* Nothing to buy... */
        return;
    }

    int qty = rand() % 5 + 1;
    int price = qty * getCostPerUnit(i);

    interface->consoleAppendText(uniqueId, QString("I would like to buy %1 of ").arg(qty) %
                                 getItemName(i) % QString(" which would cost me %1").arg(price));

    transactionMutex.lock();
    if (price > money){
        transactionMutex.unlock();
        return;
    }

    int bill = s->trade(i, qty);
    if (bill > 0) {
        money -= bill;
        stocks[i] += qty;
    }
    transactionMutex.unlock();
}
````



## Factories

`buildItem` method:

The `buildItem` method is at the core of the factory's functionality. It simulates the process of item assembly, from resource consumption to labor allocation. The implementation is simple: If the factory can pay the employees, it produces an item, then pays the employees, and once the item is produced, increments its stocks by 1.

**Note :** We must not keep the lock during the item assembly time, otherwise, we could create a contention for the other lock users.



````c++
void Factory::buildItem() {
    int salary = getEmployeeSalary(getEmployeeThatProduces(itemBuilt));

    transactionMutex.lock();
    // If we have enough money to pay salary
    if (money < salary) {
        transactionMutex.unlock();
        return;
    }

    // Produce 1 item
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
````



`orderResources` method:

Factories rely on the `orderResources` method to maintain supply of raw materials. The method will order only the resource it has the least of, simply by looking at the lowest quantity of each resource in the stocks with the `std::min_element` function. For simplicity, only 1 resource is ordered at a time. The method seeks if any wholesaler can trade the needed resource. If one of the wholesalers possesses it, the resource is purchased (decrement money and increment stocks).



````c++
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
````



## Program exit





## Tests





## Conclusion

In summary, the implementation of this lab simulation features is indicative of careful consideration of thread safety and transaction integrity. The designed `trade`, `buyResources`, `buildItem`, and `orderResources` methods function as intended, facilitating the simulation of economic interactions among extractors, wholesalers, and factories. By adhering to the principles of concurrent programming, we have ensured that the system operates consistently under various conditions.

We trust that the modifications put forth in this report meet the practical requirements of the simulation and contribute to the educational goals of the project.

