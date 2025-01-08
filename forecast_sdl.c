#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#define NK_IMPLEMENTATION
#include <nuklear.h>
#include <nuklear_sdl_renderer.h>

// Predefined parameter sets
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
} ParameterSet;

ParameterSet parameter_sets[] = {
    {2770, 100, 50, 36, 0.12, 5, 0, 1, 5, 200, 20, 0.5, 8000, 0.08, 100},
    {0, 200, 50, 36, 0.12, 5, 0, 1, 5, 200, 20, 0.5, 200, 0.02, 100}
};

const char* parameter_set_names[] = {
    "Physical Server/Co-location",
    "Cloud Server/S3"
};

void run_forecast(ParameterSet params, struct nk_context *ctx) {
    float initial_investment = params.initial_investment;
    float colocation_expense = params.colocation_expense;
    float marketing_expense = params.marketing_expense;
    int months = params.months;
    float price_per_gb = params.price_per_gb;
    float avg_gb_per_user = params.avg_gb_per_user;
    int initial_users = params.initial_users;
    int initial_companies = params.initial_companies;
    int min_employees_per_company = params.min_employees_per_company;
    int max_employees_per_company = params.max_employees_per_company;
    float acquisition_rate = params.acquisition_rate;
    float mean_company_size = params.mean_company_size;
    float cost_per_gb = params.cost_per_gb;
    float initial_storage = params.initial_storage;
    int iterations = params.iterations;

    float expenses = colocation_expense + marketing_expense;
    float cumulative_profits[iterations][months];
    float monthly_storage_usages[iterations][months];
    int break_even_months[iterations];
    int no_return_count = 0;

    int company_growth[months];
    int user_growth[months];

    for (int i = 0; i < iterations; i++) {
        company_growth[0] = initial_companies;
        user_growth[0] = initial_users;

        for (int month = 1; month < months; month++) {
            float new_companies = company_growth[month - 1] * acquisition_rate * exp(-company_growth[month - 1] / 10);
            company_growth[month] = company_growth[month - 1] + new_companies;

            float weights[max_employees_per_company - min_employees_per_company + 1];
            for (int j = 0; j < max_employees_per_company - min_employees_per_company + 1; j++) {
                weights[j] = exp(-j / mean_company_size);
            }

            float sum_weights = 0;
            for (int j = 0; j < max_employees_per_company - min_employees_per_company + 1; j++) {
                sum_weights += weights[j];
            }

            float new_users = 0;
            for (int j = 0; j < max_employees_per_company - min_employees_per_company + 1; j++) {
                new_users += new_companies * (weights[j] / sum_weights) * (min_employees_per_company + j);
            }

            user_growth[month] = user_growth[month - 1] + new_users;
        }

        for (int month = 0; month < months; month++) {
            monthly_storage_usages[i][month] = user_growth[month] * avg_gb_per_user;
        }

        float monthly_revenue[months];
        for (int month = 0; month < months; month++) {
            monthly_revenue[month] = monthly_storage_usages[i][month] * price_per_gb;
        }

        float total_monthly_expense = expenses;
        float one_time_extra_usage_cost = 0;
        int extra_usage_applied = 0;
        for (int month = 0; month < months; month++) {
            if (monthly_storage_usages[i][month] > initial_storage && !extra_usage_applied) {
                float extra_usage = monthly_storage_usages[i][month] - initial_storage;
                one_time_extra_usage_cost += extra_usage * cost_per_gb;
                extra_usage_applied = 1;
            }
        }

        float monthly_net_profit[months];
        for (int month = 0; month < months; month++) {
            monthly_net_profit[month] = monthly_revenue[month] - total_monthly_expense;
        }

        float cumulative_profit[months];
        cumulative_profit[0] = monthly_net_profit[0] - initial_investment - one_time_extra_usage_cost;
        for (int month = 1; month < months; month++) {
            cumulative_profit[month] = cumulative_profit[month - 1] + monthly_net_profit[month];
        }

        for (int month = 0; month < months; month++) {
            cumulative_profits[i][month] = cumulative_profit[month];
        }

        int break_even_month = -1;
        for (int month = 0; month < months; month++) {
            if (cumulative_profit[month] >= 0) {
                break_even_month = month;
                break;
            }
        }
        break_even_months[i] = break_even_month;
        if (break_even_month == -1) {
            no_return_count++;
        }
    }

    float avg_cumulative_profit[months];
    float std_cumulative_profit[months];
    float avg_monthly_storage_usage[months];
    float std_monthly_storage_usage[months];
    for (int month = 0; month < months; month++) {
        float sum_cumulative_profit = 0;
        float sum_monthly_storage_usage = 0;
        for (int i = 0; i < iterations; i++) {
            sum_cumulative_profit += cumulative_profits[i][month];
            sum_monthly_storage_usage += monthly_storage_usages[i][month];
        }
        avg_cumulative_profit[month] = sum_cumulative_profit / iterations;
        avg_monthly_storage_usage[month] = sum_monthly_storage_usage / iterations;

        float sum_sq_cumulative_profit = 0;
        float sum_sq_monthly_storage_usage = 0;
        for (int i = 0; i < iterations; i++) {
            sum_sq_cumulative_profit += pow(cumulative_profits[i][month] - avg_cumulative_profit[month], 2);
            sum_sq_monthly_storage_usage += pow(monthly_storage_usages[i][month] - avg_monthly_storage_usage[month], 2);
        }
        std_cumulative_profit[month] = sqrt(sum_sq_cumulative_profit / iterations);
        std_monthly_storage_usage[month] = sqrt(sum_sq_monthly_storage_usage / iterations);
    }

    float total_net_profit = avg_cumulative_profit[months - 1];
    float ROI = (total_net_profit / initial_investment) * 100;
    float risk_of_no_return = (no_return_count / (float)iterations) * 100;

    int sum_break_even_months = 0;
    int count_break_even_months = 0;
    for (int i = 0; i < iterations; i++) {
        if (break_even_months[i] != -1) {
            sum_break_even_months += break_even_months[i];
            count_break_even_months++;
        }
    }
    float avg_break_even_month = count_break_even_months > 0 ? sum_break_even_months / (float)count_break_even_months : -1;

    // Display results using Nuklear
    if (nk_begin(ctx, "Forecast Results", nk_rect(50, 50, 700, 500), NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE | NK_WINDOW_MINIMIZABLE | NK_WINDOW_TITLE)) {
        nk_layout_row_dynamic(ctx, 20, 1);
        nk_label(ctx, "Forecast Results:", NK_TEXT_LEFT);
        nk_label(ctx, "-----------------------------------", NK_TEXT_LEFT);
        nk_label(ctx, "Company and User Growth Metrics (Averaged):", NK_TEXT_LEFT);
        for (int month = 0; month < months; month++) {
            char buffer[256];
            snprintf(buffer, sizeof(buffer), "Month %d: %.0f companies, %.0f users", month + 1, company_growth[month], user_growth[month]);
            nk_label(ctx, buffer, NK_TEXT_LEFT);
        }
        nk_label(ctx, "-----------------------------------", NK_TEXT_LEFT);
        char buffer[256];
        snprintf(buffer, sizeof(buffer), "Price per GB: $%.2f", price_per_gb);
        nk_label(ctx, buffer, NK_TEXT_LEFT);
        snprintf(buffer, sizeof(buffer), "Average GB per user: %.2f GB", avg_gb_per_user);
        nk_label(ctx, buffer, NK_TEXT_LEFT);
        snprintf(buffer, sizeof(buffer), "Total Net Profit after %d months: $%.2f", months, total_net_profit);
        nk_label(ctx, buffer, NK_TEXT_LEFT);
        snprintf(buffer, sizeof(buffer), "ROI after %d months: %.2f%%", months, ROI);
        nk_label(ctx, buffer, NK_TEXT_LEFT);
        snprintf(buffer, sizeof(buffer), "Risk of No Return: %.2f%%", risk_of_no_return);
        nk_label(ctx, buffer, NK_TEXT_LEFT);
        snprintf(buffer, sizeof(buffer), "Standard Deviation of Cumulative Profit: %.2f", std_cumulative_profit[months - 1]);
        nk_label(ctx, buffer, NK_TEXT_LEFT);
        snprintf(buffer, sizeof(buffer), "Standard Deviation of Monthly Storage Usage: %.2f GB", std_monthly_storage_usage[months - 1]);
        nk_label(ctx, buffer, NK_TEXT_LEFT);
        if (avg_break_even_month != -1) {
            snprintf(buffer, sizeof(buffer), "Average Break-even at month %.0f", avg_break_even_month + 1);
            nk_label(ctx, buffer, NK_TEXT_LEFT);
        } else {
            nk_label(ctx, "No break-even point within the given timeframe.", NK_TEXT_LEFT);
        }
    }
    nk_end(ctx);
}

int main(void) {
    SDL_Window *win;
    SDL_Renderer *renderer;
    struct nk_context *ctx;
    struct nk_colorf bg;
    int running = 1;
    ParameterSet current_params = parameter_sets[0];

    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();
    win = SDL_CreateWindow("Forecast Application", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI);
    renderer = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    ctx = nk_sdl_init(win, renderer);

    struct nk_font_atlas *atlas;
    nk_sdl_font_stash_begin(&atlas);
    nk_sdl_font_stash_end();

    bg.r = 0.10f, bg.g = 0.18f, bg.b = 0.24f, bg.a = 1.0f;

    while (running) {
        SDL_Event evt;
        nk_input_begin(ctx);
        while (SDL_PollEvent(&evt)) {
            if (evt.type == SDL_QUIT) running = 0;
            nk_sdl_handle_event(&evt);
        }
        nk_input_end(ctx);

        if (nk_begin(ctx, "Forecast Parameters", nk_rect(50, 50, 700, 500), NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE | NK_WINDOW_MINIMIZABLE | NK_WINDOW_TITLE)) {
            nk_layout_row_dynamic(ctx, 30, 2);
            nk_label(ctx, "Parameter Set:", NK_TEXT_LEFT);
            if (nk_combo_begin_label(ctx, parameter_set_names[0], nk_vec2(nk_widget_width(ctx), 200))) {
                nk_layout_row_dynamic(ctx, 30, 1);
                for (int i = 0; i < sizeof(parameter_sets) / sizeof(parameter_sets[0]); i++) {
                    if (nk_combo_item_label(ctx, parameter_set_names[i], NK_TEXT_LEFT)) {
                        current_params = parameter_sets[i];
                    }
                }
                nk_combo_end(ctx);
            }

            nk_layout_row_dynamic(ctx, 30, 2);
            nk_label(ctx, "Initial Investment:", NK_TEXT_LEFT);
            nk_property_float(ctx, "#", 0, &current_params.initial_investment, 10000, 1, 1);
            nk_label(ctx, "Colocation Expense:", NK_TEXT_LEFT);
            nk_property_float(ctx, "#", 0, &current_params.colocation_expense, 1000, 1, 1);
            nk_label(ctx, "Marketing Expense:", NK_TEXT_LEFT);
            nk_property_float(ctx, "#", 0, &current_params.marketing_expense, 1000, 1, 1);
            nk_label(ctx, "Months:", NK_TEXT_LEFT);
            nk_property_int(ctx, "#", 1, &current_params.months, 120, 1, 1);
            nk_label(ctx, "Price per GB:", NK_TEXT_LEFT);
            nk_property_float(ctx, "#", 0, &current_params.price_per_gb, 10, 0.01, 0.01);
            nk_label(ctx, "Average GB per User:", NK_TEXT_LEFT);
            nk_property_float(ctx, "#", 0, &current_params.avg_gb_per_user, 100, 0.1, 0.1);
            nk_label(ctx, "Initial Users:", NK_TEXT_LEFT);
            nk_property_int(ctx, "#", 0, &current_params.initial_users, 10000, 1, 1);
            nk_label(ctx, "Initial Companies:", NK_TEXT_LEFT);
            nk_property_int(ctx, "#", 0, &current_params.initial_companies, 1000, 1, 1);
            nk_label(ctx, "Min Employees per Company:", NK_TEXT_LEFT);
            nk_property_int(ctx, "#", 1, &current_params.min_employees_per_company, 1000, 1, 1);
            nk_label(ctx, "Max Employees per Company:", NK_TEXT_LEFT);
            nk_property_int(ctx, "#", 1, &current_params.max_employees_per_company, 1000, 1, 1);
            nk_label(ctx, "Mean Company Size:", NK_TEXT_LEFT);
            nk_property_float(ctx, "#", 1, &current_params.mean_company_size, 100, 1, 1);
            nk_label(ctx, "Acquisition Rate:", NK_TEXT_LEFT);
            nk_property_float(ctx, "#", 0, &current_params.acquisition_rate, 1, 0.01, 0.01);
            nk_label(ctx, "Initial Storage (GB):", NK_TEXT_LEFT);
            nk_property_float(ctx, "#", 0, &current_params.initial_storage, 100000, 1, 1);
            nk_label(ctx, "Cost per GB extra:", NK_TEXT_LEFT);
            nk_property_float(ctx, "#", 0, &current_params.cost_per_gb, 10, 0.01, 0.01);
            nk_label(ctx, "Iterations:", NK_TEXT_LEFT);
            nk_property_int(ctx, "#", 1, &current_params.iterations, 10000, 1, 1);

            if (nk_button_label(ctx, "Run Forecast")) {
                run_forecast(current_params, ctx);
            }
        }
        nk_end(ctx);

        SDL_SetRenderDrawColor(renderer, (int)(bg.r * 255), (int)(bg.g * 255), (int)(bg.b * 255), (int)(bg.a * 255));
        SDL_RenderClear(renderer);
        nk_sdl_render(NK_ANTI_ALIASING_ON);
        SDL_RenderPresent(renderer);
    }

    nk_sdl_shutdown();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
