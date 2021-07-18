#pragma once

#include <QtGlobal>

#include <algorithm>
#include <stack>
#include <variant>
#include <vector>

namespace differ
{

namespace Private
{

/**
 * The Snake struct represents a partial match between two lists.
 */
struct Snake
{
    /**
     * Returns @c true if the snake corresponds to an addition operation.
     */
    bool isAddition() const
    {
        return x1 == x2 && y1 != y2;
    }

    /**
     * Returns @c true if the snake corresponds to a removal operation.
     */
    bool isRemoval() const
    {
        return x1 != x2 && y1 == y2;
    }

    qsizetype x1; ///< start position in the old list
    qsizetype x2; ///< end position in the old list
    qsizetype y1; ///< start position in the new list
    qsizetype y2; ///< end position in the new list
};

/**
 * The Slice struct represents a range in two lists.
 */
struct Slice
{
    bool isNull() const
    {
        return (x2 - x1) == 0 && (y2 - y1) == 0;
    }

    qsizetype x1; ///< start position in the old list
    qsizetype x2; ///< end position in the old list
    qsizetype y1; /// start position in the new list
    qsizetype y2; ///< end position in the new list
};

/**
 * Finds the middle snake in the specified @a slice. For more details, please see
 * the Myers' paper for more details.
 */
template <typename Container>
static Snake diffPartial(const Slice &slice, const Container &src, const Container &dst,
                         qsizetype *forward, qsizetype *backward, qsizetype offset)
{
    const qsizetype oldSize = slice.x2 - slice.x1;
    const qsizetype newSize = slice.y2 - slice.y1;

    if (oldSize < 1 || newSize < 1) {
        return Snake{.x1 = 0, .x2 = oldSize, .y1 = 0, .y2 = newSize};
    }

    const qsizetype delta = oldSize - newSize;
    const qsizetype max = (oldSize + newSize + 1) / 2;
    const bool front = (delta % 2) != 0;

    forward[offset + 1] = 0;
    backward[offset + 1] = newSize;

    for (qsizetype d = 0; d <= max; ++d) {
        for (qsizetype k = -d; k <= d; k += 2) {
            // Decide whether to go downward or rightward. Moving rightward means removing
            // an item from the old list; moving downward corresponds to inserting.
            qsizetype x, ox;
            if (k == -d || (k != d && forward[offset + k - 1] < forward[offset + k + 1])) {
                ox = forward[offset + k + 1];
                x = ox;
            } else {
                ox = forward[offset + k - 1];
                x = ox + 1;
            }

            // k is defined as difference between x and y.
            qsizetype y = x - k;
            const qsizetype oy = (d == 0 || x != ox) ? y : y - 1;

            // Move along the diagonals, if possible. Moving along diagonals corresponds to
            // preserving items in the old list.
            while (x < oldSize && y < newSize && src[slice.x1 + x] == dst[slice.y1 + y]) {
                ++x;
                ++y;
            }

            forward[offset + k] = x;

            const qsizetype c = k - delta;
            if (front && c >= -d + 1 && c <= d - 1 && y >= backward[offset + c]) {
                const qsizetype by = backward[offset + c];
                const qsizetype bx = by + k;

                if (x != bx) {
                    return Snake{.x1 = bx, .x2 = x, .y1 = by, .y2 = y,};
                } else {
                    return Snake{.x1 = ox, .x2 = x, .y1 = oy, .y2 = y,};
                }
            }
        }

        for (qsizetype c = -d; c <= d; c += 2) {
            // Decide whether to go leftward or upward.
            qsizetype y, oy;
            if (c == -d || (c != d && backward[offset + c - 1] > backward[offset + c + 1])) {
                oy = backward[offset + c + 1];
                y = oy;
            } else {
                oy = backward[offset + c - 1];
                y = oy - 1;
            }

            // k is defined as difference between x and y.
            const qsizetype k = c + delta;
            qsizetype x = y + k;
            const qsizetype ox = (d == 0 || y != oy) ? x : x + 1;

            // Move along the diagonals, if possible. Moving along diagonals corresponds to
            // preserving items in the old list.
            while (x > 0 && y > 0 && src[slice.x1 + x - 1] == dst[slice.y1 + y - 1]) {
                --x;
                --y;
            }

            backward[offset + c] = y;

            if (!front && k >= -d && k <= d && x <= forward[offset + k]) {
                const qsizetype fx = forward[offset + k];
                const qsizetype fy = fx - k;

                if (x != fx) {
                    return Snake{.x1 = x, .x2 = fx, .y1 = y, .y2 = fy,};
                } else {
                    return Snake{.x1 = x, .x2 = ox, .y1 = y, .y2 = oy,};
                }
            }
        }
    }

    Q_UNREACHABLE();
}

} // namespace Private

/**
 * The InsertOperation type represents an insertion operation.
 */
struct InsertOperation
{
    qsizetype index; ///< The position in the old list.
    qsizetype offset; ///< The position in the new list.
    qsizetype count; ///< The number of items to be inserted.
};

/**
 * The RemoveOperation type represents a remove operation in the old list.
 */
struct RemoveOperation
{
    qsizetype offset; ///< The position in the old list.
    qsizetype count; ///< The number of items to be removed.
};

using EditOperation = std::variant<InsertOperation, RemoveOperation>;

/**
 * This function calculates the difference between two specified lists. That's it, the
 * sequence of insert and remove operations that will transform the @a oldList into @a newList.
 *
 * If two lists are the same, an empty list will be returned. The first operation in the
 * returned list must be applied first, and the last one must be applied last.
 *
 * Internally, this function uses the Meyers' diff algorithm to calculate the difference.
 */
template <typename Container>
static std::vector<EditOperation> diff(const Container &oldList, const Container &newList)
{
    std::vector<Private::Snake> snakes;
    std::stack<Private::Slice> slices;

    const qsizetype max = oldList.size() + newList.size() + std::abs(oldList.size() - newList.size());
    std::vector<qsizetype> forward;
    std::vector<qsizetype> backward;

    forward.resize(2 * max);
    backward.resize(2 * max);

    slices.push(Private::Slice{.x1 = 0, .x2 = oldList.size(), .y1 = 0, .y2 = newList.size(),});
    while (!slices.empty()) {
        const Private::Slice slice = slices.top();
        slices.pop();

        Private::Snake snake = diffPartial(slice, oldList, newList, forward.data(), backward.data(), max);

        snake.x1 += slice.x1;
        snake.x2 += slice.x1;
        snake.y1 += slice.y1;
        snake.y2 += slice.y1;

        if (snake.isAddition() || snake.isRemoval()) {
            snakes.push_back(snake);
        }

        const Private::Slice left {
            .x1 = slice.x1,
            .x2 = snake.x1,
            .y1 = slice.y1,
            .y2 = snake.y1,
        };

        const Private::Slice right {
            .x1 = snake.x2,
            .x2 = slice.x2,
            .y1 = snake.y2,
            .y2 = slice.y2,
        };

        if (!left.isNull()) {
            slices.push(left);
        }
        if (!right.isNull()) {
            slices.push(right);
        }
    }

    std::sort(snakes.begin(), snakes.end(), [](const auto &a, const auto &b) {
        return a.x1 == b.x1 ? a.y1 < b.y1 : a.x1 < b.x1;
    });

    std::vector<EditOperation> editOperations;
    editOperations.reserve(snakes.size());

    for (auto it = snakes.crbegin(); it != snakes.crend(); ++it) {
        if (it->isAddition()) {
            editOperations.emplace_back(InsertOperation{
                .index = it->x1,
                .offset = it->y1,
                .count = it->y2 - it->y1,
            });
        } else if (it->isRemoval()) {
            editOperations.emplace_back(RemoveOperation{
                .offset = it->x1,
                .count = it->x2 - it->x1,
            });
        }
    }

    return editOperations;
}

} // namespace differ
