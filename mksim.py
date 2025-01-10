import numpy as np

class Consumer:
    def __init__(self, id, budget, preference):
        self.id = id
        self.budget = budget
        self.preference = preference  # New attribute for consumer preference

    def decide_purchase(self, product):
        return self.budget >= product.price and self.preference >= product.quality  # Consider product quality

class Product:
    def __init__(self, id, price, quality):
        self.id = id
        self.price = price
        self.quality = quality  # New attribute for product quality

class Market:
    def __init__(self, consumers, products):
        self.consumers = consumers
        self.products = products
        self.sales = {product.id: 0 for product in products}

    def simulate(self, months):
        for month in range(months):
            for consumer in self.consumers:
                for product in self.products:
                    if consumer.decide_purchase(product):
                        self.sales[product.id] += 1
                        consumer.budget -= product.price

    def report(self):
        for product_id, sales in self.sales.items():
            print(f"Product {product_id} sold {sales} units")

# Example usage
consumers = [Consumer(id=i, budget=np.random.uniform(100, 500), preference=np.random.uniform(0, 1)) for i in range(100)]
products = [Product(id=i, price=np.random.uniform(10, 50), quality=np.random.uniform(0, 1)) for i in range(5)]

market = Market(consumers, products)
market.simulate(months=12)
market.report()