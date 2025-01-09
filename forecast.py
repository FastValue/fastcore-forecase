import numpy as np
import tkinter as tk
from tkinter import ttk
from matplotlib.figure import Figure
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg

# Predefined parameter sets
parameter_sets = {
    "Physical Server/Co-location": {
        "initial_investment": 2770,
        "colocation_expense": 100,
        "marketing_expense": 50,
        "months": 36,
        "price_per_gb": 0.12,
        "avg_gb_per_user": 5,
        "initial_users": 0,
        "initial_companies": 1,
        "min_employees_per_company": 5,
        "max_employees_per_company": 200,
        "mean_company_size": 20,
        "acquisition_rate": 0.5,
        "initial_storage": 8000,
        "cost_per_gb": 0.08,
        "iterations": 100,
        "extra_gb_cost_type": "one-time"
    },
    "Cloud Server/S3": {
        "initial_investment": 0,
        "colocation_expense": 200,
        "marketing_expense": 50,
        "months": 36,
        "price_per_gb": 0.12,
        "avg_gb_per_user": 5,
        "initial_users": 0,
        "initial_companies": 1,
        "min_employees_per_company": 5,
        "max_employees_per_company": 200,
        "mean_company_size": 20,
        "acquisition_rate": 0.5,
        "initial_storage": 200,
        "cost_per_gb": 0.02,
        "iterations": 100,
        "extra_gb_cost_type": "monthly"
    }
}

# Default parameter set
default_set = "Physical Server/Co-location"

# Function to run the forecast
def run_forecast():
    global cancel_forecast
    cancel_forecast = False

    def cancel():
        global cancel_forecast
        cancel_forecast = True
        progress_window.destroy()

    initial_investment = float(initial_investment_entry.get())
    colocation_expense = float(colocation_expense_entry.get())
    marketing_expense = float(marketing_expense_entry.get())
    months = int(months_entry.get())
    price_per_gb = float(price_per_gb_entry.get())
    avg_gb_per_user = float(avg_gb_per_user_entry.get())
    initial_users = int(initial_users_entry.get())
    initial_companies = int(initial_companies_entry.get())
    min_employees_per_company = int(min_employees_entry.get())
    max_employees_per_company = int(max_employees_entry.get())
    acquisition_rate = float(acquisition_rate_entry.get())
    mean_company_size = float(mean_company_size_entry.get())
    cost_per_gb = float(cost_per_gb_entry.get())
    initial_storage = float(initial_storage_entry.get())
    iterations = int(iterations_entry.get())
    extra_gb_cost_type = extra_gb_cost_type_combobox.get()

    expenses = {
        "colocation": colocation_expense,
        "marketing": marketing_expense
    }

    cumulative_profits = []
    monthly_storage_usages = []
    break_even_months = []
    no_return_count = 0

    # Create progress window
    progress_window = tk.Toplevel(root)
    progress_window.title("Running Forecast")
    progress_label = ttk.Label(progress_window, text="Running forecast, please wait...")
    progress_label.pack(pady=10)
    progress_bar = ttk.Progressbar(progress_window, length=300, mode='determinate')
    progress_bar.pack(pady=10)
    cancel_button = ttk.Button(progress_window, text="Cancel", command=cancel)
    cancel_button.pack(pady=10)

    for i in range(iterations):
        if cancel_forecast:
            break

        company_growth = [initial_companies]
        user_growth = [initial_users]

        for month in range(1, months):
            new_companies = company_growth[-1] * acquisition_rate * np.exp(-company_growth[-1] / 10)
            company_growth.append(company_growth[-1] + new_companies)
            
            weights = np.exp(-np.arange(min_employees_per_company, max_employees_per_company + 1) / mean_company_size)
            new_users = new_companies * np.random.choice(np.arange(min_employees_per_company, max_employees_per_company + 1), p=weights/weights.sum())
            user_growth.append(user_growth[-1] + new_users)

        monthly_storage_usage = [users * avg_gb_per_user for users in user_growth]
        monthly_revenue = [usage * price_per_gb for usage in monthly_storage_usage]

        total_monthly_expense = [sum(expenses.values())] * months
        one_time_extra_usage_cost = 0
        extra_usage_applied = False
        for month, usage in enumerate(monthly_storage_usage):
            if usage > initial_storage and not extra_usage_applied:
                extra_usage = usage - initial_storage
                if extra_gb_cost_type == "one-time":
                    one_time_extra_usage_cost += extra_usage * cost_per_gb
                    extra_usage_applied = True
                elif extra_gb_cost_type == "monthly":
                    total_monthly_expense[month] += extra_usage * cost_per_gb

        monthly_net_profit = [revenue - expense for revenue, expense in zip(monthly_revenue, total_monthly_expense)]
        cumulative_profit = np.cumsum(monthly_net_profit) - initial_investment
        cumulative_profit = [profit - one_time_extra_usage_cost if i == 0 else profit for i, profit in enumerate(cumulative_profit)]

        cumulative_profits.append(cumulative_profit)
        monthly_storage_usages.append(monthly_storage_usage)
        break_even_month = next((i for i, profit in enumerate(cumulative_profit) if profit >= 0), None)
        break_even_months.append(break_even_month)
        if break_even_month is None:
            no_return_count += 1

        progress_bar['value'] = (i + 1) / iterations * 100
        progress_window.update_idletasks()

    progress_window.destroy()

    if cancel_forecast:
        results_text.delete(1.0, tk.END)
        results_text.insert(tk.END, "Forecast cancelled.\n")
        return

    avg_cumulative_profit = np.mean(cumulative_profits, axis=0)
    std_cumulative_profit = np.std(cumulative_profits, axis=0)
    avg_monthly_storage_usage = np.mean(monthly_storage_usages, axis=0)
    std_monthly_storage_usage = np.std(monthly_storage_usages, axis=0)
    avg_break_even_month = np.mean([month for month in break_even_months if month is not None])

    total_net_profit = avg_cumulative_profit[-1]
    risk_of_no_return = (no_return_count / iterations) * 100

    results_text.delete(1.0, tk.END)
    results_text.insert(tk.END, "\nCompany and User Growth Metrics (Averaged):\n")
    for month, (companies, users) in enumerate(zip(company_growth, user_growth)):
        results_text.insert(tk.END, f"Month {month + 1}: {companies:.0f} companies, {users:.0f} users\n")
    results_text.insert(tk.END, "-----------------------------------\n")
    results_text.insert(tk.END, f"Price per GB: ${price_per_gb:.2f}\n")
    results_text.insert(tk.END, f"Average GB per user: {avg_gb_per_user} GB\n")
    results_text.insert(tk.END, f"Total Net Profit after {months} months: ${total_net_profit:.2f}\n")
    results_text.insert(tk.END, f"Risk of No Return: {risk_of_no_return:.2f}%\n")
    results_text.insert(tk.END, f"Standard Deviation of Cumulative Profit: {std_cumulative_profit[-1]:.2f}\n")
    results_text.insert(tk.END, f"Standard Deviation of Monthly Storage Usage: {std_monthly_storage_usage[-1]:.2f} GB\n")

    if avg_break_even_month is not None:
        results_text.insert(tk.END, f"Average Break-even at month {avg_break_even_month + 1:.0f}\n")
    else:
        results_text.insert(tk.END, "No break-even point within the given timeframe.\n")

    results_text.see(tk.END)  # Auto scroll to the bottom

    # Clear the previous plot
    for widget in plot_frame.winfo_children():
        widget.destroy()

    # Create the plot
    fig = Figure(figsize=(6, 4), dpi=100)
    ax = fig.add_subplot(111)
    ax.plot(range(1, months + 1), avg_cumulative_profit, label="Cumulative Profit")
    ax.fill_between(range(1, months + 1), avg_cumulative_profit - std_cumulative_profit, avg_cumulative_profit + std_cumulative_profit, color='b', alpha=0.2, label="Profit Std Dev")
    ax.plot(range(1, months + 1), avg_monthly_storage_usage, label="Total GB Usage", linestyle="-.")
    ax.fill_between(range(1, months + 1), avg_monthly_storage_usage - std_monthly_storage_usage, avg_monthly_storage_usage + std_monthly_storage_usage, color='g', alpha=0.2, label="Storage Std Dev")
    ax.plot(range(1, months + 1), user_growth, label="Number of Users", linestyle="--")
    ax.axhline(0, color='red', linestyle='--', label="Break-even")
    ax.set_title("Cumulative Profit, User Growth and Total GB Usage Over Time")
    ax.set_xlabel("Months")
    ax.set_ylabel("Cumulative Profit (USD) / Users / GB")
    ax.legend()
    ax.grid()

    # Display the plot in the tkinter window
    canvas = FigureCanvasTkAgg(fig, master=plot_frame)
    canvas.draw()
    canvas.get_tk_widget().pack(fill=tk.BOTH, expand=True)

    # Create a frame for the table
    table_frame = ttk.Frame(plot_frame, height=int(root.winfo_height() * 0.25))
    table_frame.pack(fill=tk.BOTH, expand=True)

    # Create the table
    columns = ("Month", "Companies", "Users", "Storage Usage (GB)", "Revenue ($)", "Expense ($)", "Net Profit ($)", "Cumulative Profit ($)")
    table = ttk.Treeview(table_frame, columns=columns, show="headings")
    for col in columns:
        table.heading(col, text=col)
        table.column(col, anchor=tk.E, width=int(table_frame.winfo_width() / len(columns)))  # Adjust column width

    # Insert data into the table
    for month in range(months):
        table.insert("", "end", values=(
            month + 1,
            f"{company_growth[month]:.0f}",
            f"{user_growth[month]:.0f}",
            f"{avg_monthly_storage_usage[month]:.2f}",
            f"{monthly_revenue[month]:.2f}",
            f"{total_monthly_expense[month]:.2f}",
            f"{monthly_net_profit[month]:.2f}",
            f"{avg_cumulative_profit[month]:.2f}"
        ))

    table.pack(fill=tk.BOTH, expand=True)

# Function to update input fields based on selected parameter set
def update_parameters(event):
    selected_set = parameter_set_combobox.get()
    params = parameter_sets[selected_set]
    initial_investment_entry.delete(0, tk.END)
    initial_investment_entry.insert(0, params["initial_investment"])
    colocation_expense_entry.delete(0, tk.END)
    colocation_expense_entry.insert(0, params["colocation_expense"])
    marketing_expense_entry.delete(0, tk.END)
    marketing_expense_entry.insert(0, params["marketing_expense"])
    months_entry.delete(0, tk.END)
    months_entry.insert(0, params["months"])
    price_per_gb_entry.delete(0, tk.END)
    price_per_gb_entry.insert(0, params["price_per_gb"])
    avg_gb_per_user_entry.delete(0, tk.END)
    avg_gb_per_user_entry.insert(0, params["avg_gb_per_user"])
    initial_users_entry.delete(0, tk.END)
    initial_users_entry.insert(0, params["initial_users"])
    initial_companies_entry.delete(0, tk.END)
    initial_companies_entry.insert(0, params["initial_companies"])
    min_employees_entry.delete(0, tk.END)
    min_employees_entry.insert(0, params["min_employees_per_company"])
    max_employees_entry.delete(0, tk.END)
    max_employees_entry.insert(0, params["max_employees_per_company"])
    mean_company_size_entry.delete(0, tk.END)
    mean_company_size_entry.insert(0, params["mean_company_size"])
    acquisition_rate_entry.delete(0, tk.END)
    acquisition_rate_entry.insert(0, params["acquisition_rate"])
    initial_storage_entry.delete(0, tk.END)
    initial_storage_entry.insert(0, params["initial_storage"])
    cost_per_gb_entry.delete(0, tk.END)
    cost_per_gb_entry.insert(0, params["cost_per_gb"])
    iterations_entry.delete(0, tk.END)
    iterations_entry.insert(0, params["iterations"])
    extra_gb_cost_type_combobox.set(params["extra_gb_cost_type"])

# Create the main window
root = tk.Tk()
root.title("Forecast Application")

# Configure grid weights for responsiveness
root.grid_columnconfigure(0, weight=0)
root.grid_columnconfigure(1, weight=0)
root.grid_columnconfigure(2, weight=4)
root.grid_rowconfigure(18, weight=1)

# Create and place the parameter set dropdown
ttk.Label(root, text="Parameter Set:").grid(row=0, column=0, sticky="ew")
parameter_set_combobox = ttk.Combobox(root, values=list(parameter_sets.keys()))
parameter_set_combobox.grid(row=0, column=1, sticky="ew")
parameter_set_combobox.bind("<<ComboboxSelected>>", update_parameters)
parameter_set_combobox.current(0)  # Set default selection

# Create and place the input fields with default values
ttk.Label(root, text="Initial Investment:").grid(row=1, column=0, sticky="ew")
initial_investment_entry = ttk.Entry(root)
initial_investment_entry.insert(0, parameter_sets[default_set]["initial_investment"])
initial_investment_entry.grid(row=1, column=1, sticky="ew")

ttk.Label(root, text="Colocation Expense:").grid(row=2, column=0, sticky="ew")
colocation_expense_entry = ttk.Entry(root)
colocation_expense_entry.insert(0, parameter_sets[default_set]["colocation_expense"])
colocation_expense_entry.grid(row=2, column=1, sticky="ew")

ttk.Label(root, text="Marketing Expense:").grid(row=3, column=0, sticky="ew")
marketing_expense_entry = ttk.Entry(root)
marketing_expense_entry.insert(0, parameter_sets[default_set]["marketing_expense"])
marketing_expense_entry.grid(row=3, column=1, sticky="ew")

ttk.Label(root, text="Months:").grid(row=4, column=0, sticky="ew")
months_entry = ttk.Entry(root)
months_entry.insert(0, parameter_sets[default_set]["months"])
months_entry.grid(row=4, column=1, sticky="ew")

ttk.Label(root, text="Price per GB:").grid(row=5, column=0, sticky="ew")
price_per_gb_entry = ttk.Entry(root)
price_per_gb_entry.insert(0, parameter_sets[default_set]["price_per_gb"])
price_per_gb_entry.grid(row=5, column=1, sticky="ew")

ttk.Label(root, text="Average GB per User:").grid(row=6, column=0, sticky="ew")
avg_gb_per_user_entry = ttk.Entry(root)
avg_gb_per_user_entry.insert(0, parameter_sets[default_set]["avg_gb_per_user"])
avg_gb_per_user_entry.grid(row=6, column=1, sticky="ew")

ttk.Label(root, text="Initial Users:").grid(row=7, column=0, sticky="ew")
initial_users_entry = ttk.Entry(root)
initial_users_entry.insert(0, parameter_sets[default_set]["initial_users"])
initial_users_entry.grid(row=7, column=1, sticky="ew")

ttk.Label(root, text="Initial Companies:").grid(row=8, column=0, sticky="ew")
initial_companies_entry = ttk.Entry(root)
initial_companies_entry.insert(0, parameter_sets[default_set]["initial_companies"])
initial_companies_entry.grid(row=8, column=1, sticky="ew")

ttk.Label(root, text="Min Employees per Company:").grid(row=9, column=0, sticky="ew")
min_employees_entry = ttk.Entry(root)
min_employees_entry.insert(0, parameter_sets[default_set]["min_employees_per_company"])
min_employees_entry.grid(row=9, column=1, sticky="ew")

ttk.Label(root, text="Max Employees per Company:").grid(row=10, column=0, sticky="ew")
max_employees_entry = ttk.Entry(root)
max_employees_entry.insert(0, parameter_sets[default_set]["max_employees_per_company"])
max_employees_entry.grid(row=10, column=1, sticky="ew")

ttk.Label(root, text="Mean Company Size:").grid(row=11, column=0, sticky="ew")
mean_company_size_entry = ttk.Entry(root)
mean_company_size_entry.insert(0, parameter_sets[default_set]["mean_company_size"])
mean_company_size_entry.grid(row=11, column=1, sticky="ew")

ttk.Label(root, text="Acquisition Rate:").grid(row=12, column=0, sticky="ew")
acquisition_rate_entry = ttk.Entry(root)
acquisition_rate_entry.insert(0, parameter_sets[default_set]["acquisition_rate"])
acquisition_rate_entry.grid(row=12, column=1, sticky="ew")

ttk.Label(root, text="Initial Storage (GB):").grid(row=13, column=0, sticky="ew")
initial_storage_entry = ttk.Entry(root)
initial_storage_entry.insert(0, parameter_sets[default_set]["initial_storage"])
initial_storage_entry.grid(row=13, column=1, sticky="ew")

ttk.Label(root, text="Cost per GB extra:").grid(row=14, column=0, sticky="ew")
cost_per_gb_entry = ttk.Entry(root)
cost_per_gb_entry.insert(0, parameter_sets[default_set]["cost_per_gb"])
cost_per_gb_entry.grid(row=14, column=1, sticky="ew")

ttk.Label(root, text="Extra GB Cost Type:").grid(row=15, column=0, sticky="ew")
extra_gb_cost_type_combobox = ttk.Combobox(root, values=["one-time", "monthly"])
extra_gb_cost_type_combobox.grid(row=15, column=1, sticky="ew")
extra_gb_cost_type_combobox.set(parameter_sets[default_set]["extra_gb_cost_type"])

ttk.Label(root, text="Iterations:").grid(row=16, column=0, sticky="ew")
iterations_entry = ttk.Entry(root)
iterations_entry.insert(0, parameter_sets[default_set]["iterations"])
iterations_entry.grid(row=16, column=1, sticky="ew")

# Create and place the run button
run_button = ttk.Button(root, text="Run Forecast", command=run_forecast)
run_button.grid(row=17, column=0, columnspan=2, sticky="ew")

# Create and place the results text box
results_text = tk.Text(root, height=15, width=50)
results_text.grid(row=18, column=0, columnspan=2, sticky="nsew")

# Create and place the plot frame
plot_frame = ttk.Frame(root, width=800)
plot_frame.grid(row=0, column=2, rowspan=19, sticky="nsew")  # Adjust rowspan to 17

# Run the main loop
root.mainloop()
