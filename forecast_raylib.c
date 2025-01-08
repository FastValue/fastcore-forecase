#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#include "raylib.h"

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#define MAX_MONTHS 36
#define MAX_ITERATIONS 100

typedef struct {
    float initial_investment;
    float colocation_expense;
    float marketing_expense;
    int months;
    float price_per_gb;
    float avg_gb_per_user;
    int initial_users;
    int initial_companies;
    int min_employees_per_company;
    int max_employees_per_company;
    float mean_company_size;
    float acquisition_rate;
    float initial_storage;
    float cost_per_gb;
    int iterations;
} Parameters;

Parameters parameter_sets[] = {
    {2770, 100, 50, 36, 0.12, 5, 0, 1, 5, 200, 20, 0.5, 8000, 0.08, 100},
    {0, 200, 50, 36, 0.12, 5, 0, 1, 5, 200, 20, 0.5, 200, 0.02, 100}
};

const char *parameter_set_names[] = {"Physical Server/Co-location", "Cloud Server/S3"};
int selected_set = 0;
Parameters params;

void UpdateParameters() {
    params = parameter_sets[selected_set];
}

float company_growth[MAX_MONTHS];
float user_growth[MAX_MONTHS];
float monthly_storage_usage[MAX_MONTHS];
float monthly_revenue[MAX_MONTHS];
float monthly_net_profit[MAX_MONTHS];
float cumulative_profit[MAX_MONTHS];

void RunForecast() {
    float company_growth[MAX_MONTHS] = {params.initial_companies};
    float user_growth[MAX_MONTHS] = {params.initial_users};
    float monthly_storage_usage[MAX_MONTHS];
    float monthly_revenue[MAX_MONTHS];
    float monthly_net_profit[MAX_MONTHS];
    float cumulative_profit[MAX_MONTHS];
    float total_monthly_expense = params.colocation_expense + params.marketing_expense;
    float one_time_extra_usage_cost = 0;
    bool extra_usage_applied = false;

    for (int month = 1; month < params.months; month++) {
        float new_companies = company_growth[month - 1] * params.acquisition_rate * exp(-company_growth[month - 1] / 10);
        company_growth[month] = company_growth[month - 1] + new_companies;

        float weights[params.max_employees_per_company - params.min_employees_per_company + 1];
        float sum_weights = 0;
        for (int i = 0; i < params.max_employees_per_company - params.min_employees_per_company + 1; i++) {
            weights[i] = exp(-(params.min_employees_per_company + i) / params.mean_company_size);
            sum_weights += weights[i];
        }

        float new_users = 0;
        for (int i = 0; i < new_companies; i++) {
            float rand_val = (float)rand() / RAND_MAX;
            float cumulative_weight = 0;
            for (int j = 0; j < params.max_employees_per_company - params.min_employees_per_company + 1; j++) {
                cumulative_weight += weights[j] / sum_weights;
                if (rand_val <= cumulative_weight) {
                    new_users += params.min_employees_per_company + j;
                    break;
                }
            }
        }
        user_growth[month] = user_growth[month - 1] + new_users;
    }

    for (int month = 0; month < params.months; month++) {
        monthly_storage_usage[month] = user_growth[month] * params.avg_gb_per_user;
        monthly_revenue[month] = monthly_storage_usage[month] * params.price_per_gb;

        if (monthly_storage_usage[month] > params.initial_storage && !extra_usage_applied) {
            float extra_usage = monthly_storage_usage[month] - params.initial_storage;
            one_time_extra_usage_cost += extra_usage * params.cost_per_gb;
            extra_usage_applied = true;
        }

        monthly_net_profit[month] = monthly_revenue[month] - total_monthly_expense;
        cumulative_profit[month] = (month == 0 ? 0 : cumulative_profit[month - 1]) + monthly_net_profit[month];
        if (month == 0) {
            cumulative_profit[month] -= params.initial_investment + one_time_extra_usage_cost;
        }
    }

    // Display results (placeholder)
    for (int month = 0; month < params.months; month++) {
        printf("Month %d: Companies: %.2f, Users: %.2f, Storage Usage: %.2f GB, Revenue: $%.2f, Net Profit: $%.2f, Cumulative Profit: $%.2f\n",
               month + 1, company_growth[month], user_growth[month], monthly_storage_usage[month], monthly_revenue[month], monthly_net_profit[month], cumulative_profit[month]);
    }
}

void DrawChart() {
    int chartWidth = 800;
    int chartHeight = 400;
    int chartX = 300;
    int chartY = 50;
    int maxMonths = params.months;
    float maxProfit = cumulative_profit[0];
    float minProfit = cumulative_profit[0];
    float maxStorage = monthly_storage_usage[0];
    float maxUsers = user_growth[0];

    for (int i = 1; i < maxMonths; i++) {
        if (cumulative_profit[i] > maxProfit) maxProfit = cumulative_profit[i];
        if (cumulative_profit[i] < minProfit) minProfit = cumulative_profit[i];
        if (monthly_storage_usage[i] > maxStorage) maxStorage = monthly_storage_usage[i];
        if (user_growth[i] > maxUsers) maxUsers = user_growth[i];
    }

    DrawRectangle(chartX, chartY, chartWidth, chartHeight, LIGHTGRAY);
    DrawLine(chartX, chartY + chartHeight / 2, chartX + chartWidth, chartY + chartHeight / 2, DARKGRAY); // X-axis

    for (int i = 0; i < maxMonths - 1; i++) {
        int x1 = chartX + (i * chartWidth / maxMonths);
        int x2 = chartX + ((i + 1) * chartWidth / maxMonths);

        int y1_profit = chartY + chartHeight / 2 - (cumulative_profit[i] * chartHeight / (2 * maxProfit));
        int y2_profit = chartY + chartHeight / 2 - (cumulative_profit[i + 1] * chartHeight / (2 * maxProfit));
        DrawLine(x1, y1_profit, x2, y2_profit, BLUE);

        int y1_storage = chartY + chartHeight - (monthly_storage_usage[i] * chartHeight / maxStorage);
        int y2_storage = chartY + chartHeight - (monthly_storage_usage[i + 1] * chartHeight / maxStorage);
        DrawLine(x1, y1_storage, x2, y2_storage, GREEN);

        int y1_users = chartY + chartHeight - (user_growth[i] * chartHeight / maxUsers);
        int y2_users = chartY + chartHeight - (user_growth[i + 1] * chartHeight / maxUsers);
        DrawLine(x1, y1_users, x2, y2_users, RED);
    }

    DrawText("Cumulative Profit", chartX + 10, chartY + 10, 10, BLUE);
    DrawText("Monthly Storage Usage", chartX + 10, chartY + 30, 10, GREEN);
    DrawText("User Growth", chartX + 10, chartY + 50, 10, RED);
}

int main(void) {
    InitWindow(1200, 800, "Forecast Application");
    SetTargetFPS(60);

    UpdateParameters();

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(RAYWHITE);

        GuiLabel((Rectangle){20, 20, 200, 20}, "Parameter Set:");
        if (GuiDropdownBox((Rectangle){230, 20, 200, 20}, "Physical Server/Co-location;Cloud Server/S3", &selected_set, false)) {
            UpdateParameters();
        }

        params.initial_investment = GuiSlider((Rectangle){20, 60, 200, 20}, "Initial Investment", TextFormat("%.2f", params.initial_investment), params.initial_investment, 0, 10000);
        params.colocation_expense = GuiSlider((Rectangle){20, 100, 200, 20}, "Colocation Expense", TextFormat("%.2f", params.colocation_expense), params.colocation_expense, 0, 1000);
        params.marketing_expense = GuiSlider((Rectangle){20, 140, 200, 20}, "Marketing Expense", TextFormat("%.2f", params.marketing_expense), params.marketing_expense, 0, 1000);
        params.months = GuiSlider((Rectangle){20, 180, 200, 20}, "Months", TextFormat("%d", params.months), params.months, 1, MAX_MONTHS);
        params.price_per_gb = GuiSlider((Rectangle){20, 220, 200, 20}, "Price per GB", TextFormat("%.2f", params.price_per_gb), params.price_per_gb, 0, 1);
        params.avg_gb_per_user = GuiSlider((Rectangle){20, 260, 200, 20}, "Average GB per User", TextFormat("%.2f", params.avg_gb_per_user), params.avg_gb_per_user, 0, 100);
        params.initial_users = GuiSlider((Rectangle){20, 300, 200, 20}, "Initial Users", TextFormat("%d", params.initial_users), params.initial_users, 0, 1000);
        params.initial_companies = GuiSlider((Rectangle){20, 340, 200, 20}, "Initial Companies", TextFormat("%d", params.initial_companies), params.initial_companies, 0, 100);
        params.min_employees_per_company = GuiSlider((Rectangle){20, 380, 200, 20}, "Min Employees per Company", TextFormat("%d", params.min_employees_per_company), params.min_employees_per_company, 1, 1000);
        params.max_employees_per_company = GuiSlider((Rectangle){20, 420, 200, 20}, "Max Employees per Company", TextFormat("%d", params.max_employees_per_company), params.max_employees_per_company, 1, 1000);
        params.mean_company_size = GuiSlider((Rectangle){20, 460, 200, 20}, "Mean Company Size", TextFormat("%.2f", params.mean_company_size), params.mean_company_size, 1, 100);
        params.acquisition_rate = GuiSlider((Rectangle){20, 500, 200, 20}, "Acquisition Rate", TextFormat("%.2f", params.acquisition_rate), params.acquisition_rate, 0, 1);
        params.initial_storage = GuiSlider((Rectangle){20, 540, 200, 20}, "Initial Storage (GB)", TextFormat("%.2f", params.initial_storage), params.initial_storage, 0, 10000);
        params.cost_per_gb = GuiSlider((Rectangle){20, 580, 200, 20}, "Cost per GB extra", TextFormat("%.2f", params.cost_per_gb), params.cost_per_gb, 0, 1);
        params.iterations = GuiSlider((Rectangle){20, 620, 200, 20}, "Iterations", TextFormat("%d", params.iterations), params.iterations, 1, MAX_ITERATIONS);

        if (GuiButton((Rectangle){20, 660, 200, 30}, "Run Forecast")) {
            RunForecast();
        }

        DrawChart();

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
