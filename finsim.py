import numpy as np

class ProjectParameters:
    def __init__(self, initial_expenses, monthly_expenses, extra_expenses_conditions, revenue_model, months):
        self.initial_expenses = initial_expenses
        self.monthly_expenses = monthly_expenses
        self.extra_expenses_conditions = extra_expenses_conditions
        self.revenue_model = revenue_model
        self.months = months

class Expense:
    def __init__(self, amount):
        self.amount = amount

    def calculate(self, month, context):
        return self.amount

class ConditionalExpense(Expense):
    def __init__(self, amount, condition):
        super().__init__(amount)
        self.condition = condition

    def calculate(self, month, context):
        if self.condition(context):
            return self.amount
        return 0

class Revenue:
    def __init__(self, base_amount):
        self.base_amount = base_amount

    def calculate(self, month, context):
        return self.base_amount

class SimulationEngine:
    def __init__(self, parameters):
        self.parameters = parameters
        self.cumulative_profit = []

    def run(self):
        context = {'month': 0}
        total_expenses = sum(expense.calculate(0, context) for expense in self.parameters.initial_expenses)
        cumulative_profit = -total_expenses

        for month in range(1, self.parameters.months + 1):
            context = {'month': month}
            monthly_expenses = sum(expense.calculate(month, context) for expense in self.parameters.monthly_expenses)
            extra_expenses = sum(expense.calculate(month, context) for expense in self.parameters.extra_expenses_conditions)
            revenue = self.parameters.revenue_model.calculate(month, context)

            net_profit = revenue - (monthly_expenses + extra_expenses)
            cumulative_profit += net_profit
            self.cumulative_profit.append(cumulative_profit)

        return self.cumulative_profit

class CustomerBasedRevenue(Revenue):
    def __init__(self, initial_customers, customer_growth_rate, revenue_per_customer):
        self.initial_customers = initial_customers
        self.customer_growth_rate = customer_growth_rate
        self.revenue_per_customer = revenue_per_customer

    def calculate(self, month, context):
        # Calculate the number of customers for the given month
        customers = self.initial_customers * (1 + self.customer_growth_rate) ** month
        # Calculate revenue based on the number of customers
        return customers * self.revenue_per_customer

class CloudStorageService:
    def __init__(self, initial_customers, customer_growth_rate, revenue_per_gb, monthly_server_cost, s3_storage_cost_per_gb):
        self.initial_customers = initial_customers
        self.customer_growth_rate = customer_growth_rate
        self.revenue_per_gb = revenue_per_gb
        self.monthly_server_cost = monthly_server_cost
        self.s3_storage_cost_per_gb = s3_storage_cost_per_gb

    def calculate_customers(self, month):
        # Realistic customer growth model
        return self.initial_customers * (1 + self.customer_growth_rate) ** month

    def calculate_revenue(self, month, context):
        customers = self.calculate_customers(month)
        average_storage_per_customer = 50  # Assume an average of 50GB per customer
        total_storage = customers * average_storage_per_customer
        return total_storage * self.revenue_per_gb

    def calculate_expenses(self, month, context):
        customers = self.calculate_customers(month)
        average_storage_per_customer = 50  # Assume an average of 50GB per customer
        total_storage = customers * average_storage_per_customer
        s3_storage_cost = total_storage * self.s3_storage_cost_per_gb
        return self.monthly_server_cost + s3_storage_cost

# Example usage
initial_customers = 100
customer_growth_rate = 0.05
revenue_per_gb = 0.10
monthly_server_cost = 200
s3_storage_cost_per_gb = 0.02

cloud_service = CloudStorageService(initial_customers, customer_growth_rate, revenue_per_gb, monthly_server_cost, s3_storage_cost_per_gb)

initial_expenses = [Expense(cloud_service.monthly_server_cost)]
monthly_expenses = [Expense(cloud_service.monthly_server_cost)]
extra_expenses_conditions = [ConditionalExpense(cloud_service.s3_storage_cost_per_gb * 50, lambda context: True)]
revenue_model = Revenue(cloud_service.calculate_revenue(0, {}))

parameters = ProjectParameters(initial_expenses, monthly_expenses, extra_expenses_conditions, revenue_model, 36)

engine = SimulationEngine(parameters)
cumulative_profit = engine.run()
print(cumulative_profit)