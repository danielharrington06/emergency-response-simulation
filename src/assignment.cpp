#include "../include/assignment.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <vector>

std::vector<Assignment> findOptimalAssignment(const std::vector<std::vector<double>>& costs) {
    if (costs.empty()) {
        return {};
    }

    const std::size_t rows = costs.size();
    const std::size_t columns = costs.at(0).size();

    if (columns == 0) {
        return {};
    }

    for (const auto& row : costs) {
        if (row.size() != columns) {
            throw std::invalid_argument("Assignment cost matrix must be rectangular");
        }
    }

    // Hungarian algorithm expects rows <= columns
    // If there are more vehicles than incidents, transpose the matrix and swap the indices when creating assignments
    bool transposed = false;

    std::vector<std::vector<double>> matrix = costs;

    if (rows > columns) {
        transposed = true;

        matrix.assign(columns, std::vector<double>(rows));

        for (std::size_t i = 0; i < rows; i++) {
            for (std::size_t j = 0; j < columns; j++) {
                matrix[j][i] = costs[i][j];
            }
        }
    }

    const std::size_t n = matrix.size();
    const std::size_t m = matrix.at(0).size();

    // A sufficiently large finite value used instead of infinity.
    constexpr double LARGE_COST = 1e9;

    for (std::size_t i = 0; i < n; i++) {
        for (std::size_t j = 0; j < m; j++) {
            if (!std::isfinite(matrix[i][j])) {
                matrix[i][j] = LARGE_COST;
            }
        }
    }

    // ! Hungarian algorithm:
    // u and v are the dual potentials
    // p[j] stores the row currently assigned to column j
    // way[j] stores the previous column used to construct the augmenting path

    std::vector<double> u(n + 1);
    std::vector<double> v(m + 1);

    std::vector<std::size_t> p(m + 1);
    std::vector<std::size_t> way(m + 1);

    for (std::size_t i = 1; i <= n; i++) {
        p[0] = i;

        std::size_t j0 = 0;

        std::vector<double> minValue(m + 1, std::numeric_limits<double>::infinity());

        std::vector<bool> used(m + 1, false);

        do {
            used[j0] = true;

            std::size_t i0 = p[j0];
            double delta = std::numeric_limits<double>::infinity();

            std::size_t j1 = 0;

            for (std::size_t j = 1; j <= m; j++) {
                if (used[j]) {
                    continue;
                }

                double current = matrix[i0 - 1][j - 1] - u[i0] - v[j];

                if (current < minValue[j]) {
                    minValue[j] = current;
                    way[j] = j0;
                }

                if (minValue[j] < delta) {
                    delta = minValue[j];
                    j1 = j;
                }
            }

            for (std::size_t j = 0; j <= m; j++) {
                if (used[j]) {
                    u[p[j]] += delta;
                    v[j] -= delta;
                }
                else {
                    minValue[j] -= delta;
                }
            }

            j0 = j1;

        } while (p[j0] != 0);

        do {
            std::size_t j1 = way[j0];

            p[j0] = p[j1];

            j0 = j1;

        } while (j0 != 0);
    }

    std::vector<Assignment> assignments;

    for (std::size_t j = 1; j <= m; j++) {
        if (p[j] == 0) {
            continue;
        }

        std::size_t row = p[j] - 1;
        std::size_t column = j - 1;

        // Do not return assignments that were originally
        // impossible.
        if (!std::isfinite(transposed ? costs[column][row] : costs[row][column])) {
            continue;
        }

        if (transposed) {
            assignments.push_back({
                .vehicleIndex = column,
                .incidentIndex = row
            });
        }
        else {
            assignments.push_back({
                .vehicleIndex = row,
                .incidentIndex = column
            });
        }
    }

    return assignments;
}