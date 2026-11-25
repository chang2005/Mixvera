#include "mixvera/ui/media/qr_encoder.hpp"

#include <QString>

#include <algorithm>
#include <array>
#include <cmath>

namespace mixvera::ui {

namespace {

// ISO/IEC 18004 error-correction level M block layout, versions 1-10.
struct VersionSpec {
    int ecPerBlock;
    int group1Blocks;
    int group1Data;
    int group2Blocks;
    int group2Data;
};

constexpr std::array<VersionSpec, 10> kVersions = { {
    { 10, 1, 16, 0, 0 },   // 1
    { 16, 1, 28, 0, 0 },   // 2
    { 26, 1, 44, 0, 0 },   // 3
    { 18, 2, 32, 0, 0 },   // 4
    { 24, 2, 43, 0, 0 },   // 5
    { 16, 4, 27, 0, 0 },   // 6
    { 18, 4, 31, 0, 0 },   // 7
    { 22, 2, 38, 2, 39 },  // 8
    { 22, 3, 36, 2, 37 },  // 9
    { 26, 4, 43, 1, 44 },  // 10
} };

const std::array<std::array<int, 3>, 10> kAlignment = { {
    { 0, 0, 0 },
    { 6, 18, 0 },
    { 6, 22, 0 },
    { 6, 26, 0 },
    { 6, 30, 0 },
    { 6, 34, 0 },
    { 6, 22, 38 },
    { 6, 24, 42 },
    { 6, 26, 46 },
    { 6, 28, 50 },
} };

int alignmentCount(int version)
{
    if (version <= 1) {
        return 0;
    }
    if (version <= 6) {
        return 2;
    }
    return 3;
}

struct GaloisField {
    std::array<int, 512> exp{};
    std::array<int, 256> log{};

    GaloisField()
    {
        int value = 1;
        for (int index = 0; index < 255; ++index) {
            exp[index] = value;
            log[value] = index;
            value <<= 1;
            if (value & 0x100) {
                value ^= 0x11D;
            }
        }
        for (int index = 255; index < 512; ++index) {
            exp[index] = exp[index - 255];
        }
    }

    int multiply(int left, int right) const
    {
        if (left == 0 || right == 0) {
            return 0;
        }
        return exp[log[left] + log[right]];
    }
};

const GaloisField& field()
{
    static const GaloisField instance;
    return instance;
}

QVector<int> reedSolomonGenerator(int degree)
{
    const GaloisField& gf = field();
    QVector<int> generator{ 1 };
    for (int index = 0; index < degree; ++index) {
        QVector<int> next(generator.size() + 1, 0);
        for (int coefficient = 0; coefficient < generator.size(); ++coefficient) {
            next[coefficient] ^= generator.at(coefficient);
            next[coefficient + 1] ^= gf.multiply(generator.at(coefficient), gf.exp[index]);
        }
        generator = next;
    }
    return generator;
}

QVector<int> reedSolomonRemainder(const QVector<int>& data, int degree)
{
    const GaloisField& gf = field();
    const QVector<int> generator = reedSolomonGenerator(degree);
    QVector<int> remainder = data;
    remainder.resize(data.size() + degree, 0);
    for (int index = 0; index < data.size(); ++index) {
        const int factor = remainder.at(index);
        if (factor == 0) {
            continue;
        }
        for (int coefficient = 0; coefficient < generator.size(); ++coefficient) {
            remainder[index + coefficient]
                ^= gf.multiply(generator.at(coefficient), factor);
        }
    }
    return remainder.mid(data.size());
}

bool maskBit(int mask, int row, int column)
{
    switch (mask) {
    case 0:
        return (row + column) % 2 == 0;
    case 1:
        return row % 2 == 0;
    case 2:
        return column % 3 == 0;
    case 3:
        return (row + column) % 3 == 0;
    case 4:
        return (row / 2 + column / 3) % 2 == 0;
    case 5:
        return (row * column) % 2 + (row * column) % 3 == 0;
    case 6:
        return ((row * column) % 2 + (row * column) % 3) % 2 == 0;
    default:
        return ((row + column) % 2 + (row * column) % 3) % 2 == 0;
    }
}

int formatBits(int mask)
{
    const int data = mask; // error-correction level M is 00
    int remainder = data << 10;
    for (int bit = 14; bit >= 10; --bit) {
        if (remainder & (1 << bit)) {
            remainder ^= 0x537 << (bit - 10);
        }
    }
    return ((data << 10) | (remainder & 0x3FF)) ^ 0x5412;
}

int versionBits(int version)
{
    int remainder = version << 12;
    for (int bit = 17; bit >= 12; --bit) {
        if (remainder & (1 << bit)) {
            remainder ^= 0x1F25 << (bit - 12);
        }
    }
    return (version << 12) | (remainder & 0xFFF);
}

int penaltyScore(const QVector<QVector<int>>& matrix)
{
    const int size = matrix.size();
    int score = 0;

    const auto runPenalty = [&](const QVector<int>& line) {
        int runLength = 1;
        for (int index = 1; index <= line.size(); ++index) {
            if (index < line.size() && line.at(index) == line.at(index - 1)) {
                ++runLength;
                continue;
            }
            if (runLength >= 5) {
                score += 3 + runLength - 5;
            }
            runLength = 1;
        }
        static const std::array<int, 11> first = { 1, 0, 1, 1, 1, 0, 1, 0, 0, 0, 0 };
        static const std::array<int, 11> second = { 0, 0, 0, 0, 1, 0, 1, 1, 1, 0, 1 };
        for (int start = 0; start + 11 <= line.size(); ++start) {
            bool matchesFirst = true;
            bool matchesSecond = true;
            for (int offset = 0; offset < 11; ++offset) {
                const int value = line.at(start + offset);
                matchesFirst = matchesFirst && value == first[offset];
                matchesSecond = matchesSecond && value == second[offset];
            }
            if (matchesFirst || matchesSecond) {
                score += 40;
            }
        }
    };

    for (int row = 0; row < size; ++row) {
        runPenalty(matrix.at(row));
    }
    for (int column = 0; column < size; ++column) {
        QVector<int> line(size);
        for (int row = 0; row < size; ++row) {
            line[row] = matrix.at(row).at(column);
        }
        runPenalty(line);
    }

    for (int row = 0; row + 1 < size; ++row) {
        for (int column = 0; column + 1 < size; ++column) {
            const int value = matrix.at(row).at(column);
            if (value == matrix.at(row).at(column + 1)
                && value == matrix.at(row + 1).at(column)
                && value == matrix.at(row + 1).at(column + 1)) {
                score += 3;
            }
        }
    }

    int dark = 0;
    for (const auto& row : matrix) {
        for (const int value : row) {
            dark += value;
        }
    }
    const int total = size * size;
    const int percentTwice = dark * 200 / total;
    score += 10 * (std::abs(percentTwice - 100) / 10);
    return score;
}

void placeFinder(QVector<QVector<int>>& matrix, QVector<QVector<bool>>& reserved, int top,
    int left)
{
    const int size = matrix.size();
    for (int row = -1; row <= 7; ++row) {
        for (int column = -1; column <= 7; ++column) {
            const int targetRow = top + row;
            const int targetColumn = left + column;
            if (targetRow < 0 || targetRow >= size || targetColumn < 0
                || targetColumn >= size) {
                continue;
            }
            int value = 0;
            if (row >= 0 && row <= 6 && column >= 0 && column <= 6) {
                const bool border = row == 0 || row == 6 || column == 0 || column == 6;
                const bool centre = row >= 2 && row <= 4 && column >= 2 && column <= 4;
                value = (border || centre) ? 1 : 0;
            }
            matrix[targetRow][targetColumn] = value;
            reserved[targetRow][targetColumn] = true;
        }
    }
}

void placeAlignment(QVector<QVector<int>>& matrix, QVector<QVector<bool>>& reserved, int centreRow,
    int centreColumn)
{
    const int size = matrix.size();
    for (int row = -2; row <= 2; ++row) {
        for (int column = -2; column <= 2; ++column) {
            const int targetRow = centreRow + row;
            const int targetColumn = centreColumn + column;
            if (targetRow < 0 || targetRow >= size || targetColumn < 0
                || targetColumn >= size) {
                continue;
            }
            const int distance = std::max(std::abs(row), std::abs(column));
            matrix[targetRow][targetColumn] = distance == 1 ? 0 : 1;
            reserved[targetRow][targetColumn] = true;
        }
    }
}

void placeFormat(QVector<QVector<int>>& matrix, int mask)
{
    const int size = matrix.size();
    const int bits = formatBits(mask);
    const auto bit = [bits](int index) { return (bits >> index) & 1; };
    for (int index = 0; index <= 5; ++index) {
        matrix[8][index] = bit(index);
    }
    matrix[8][7] = bit(6);
    matrix[8][8] = bit(7);
    matrix[7][8] = bit(8);
    for (int index = 9; index <= 14; ++index) {
        matrix[14 - index][8] = bit(index);
    }
    for (int index = 0; index <= 7; ++index) {
        matrix[8][size - 1 - index] = bit(index);
    }
    for (int index = 8; index <= 14; ++index) {
        matrix[size - 15 + index][8] = bit(index);
    }
}

QVector<QVector<bool>> qrMatrixInternal(const QByteArray& payload)
{
    const int length = payload.size();
    int version = 0;
    for (int candidate = 1; candidate <= 10; ++candidate) {
        const VersionSpec& spec = kVersions.at(candidate - 1);
        const int dataCodewords = spec.group1Blocks * spec.group1Data
            + spec.group2Blocks * spec.group2Data;
        const int countBits = candidate <= 9 ? 8 : 16;
        const int needed = 4 + countBits + length * 8;
        if (needed <= dataCodewords * 8) {
            version = candidate;
            break;
        }
    }
    if (version == 0) {
        return {};
    }

    const VersionSpec& spec = kVersions.at(version - 1);
    const int dataCodewords = spec.group1Blocks * spec.group1Data
        + spec.group2Blocks * spec.group2Data;
    const int countBits = version <= 9 ? 8 : 16;

    QVector<bool> bits;
    const auto append = [&bits](int value, int count) {
        for (int index = count - 1; index >= 0; --index) {
            bits.append(((value >> index) & 1) != 0);
        }
    };
    append(4, 4); // byte mode
    append(length, countBits);
    for (const char byte : payload) {
        append(static_cast<unsigned char>(byte), 8);
    }
    const int capacity = dataCodewords * 8;
    const int terminator = std::min(4, static_cast<int>(capacity - bits.size()));
    for (int index = 0; index < terminator; ++index) {
        bits.append(false);
    }
    while (bits.size() % 8 != 0) {
        bits.append(false);
    }
    QVector<int> dataBytes;
    for (int index = 0; index + 7 < bits.size(); index += 8) {
        int value = 0;
        for (int offset = 0; offset < 8; ++offset) {
            value = (value << 1) | (bits.at(index + offset) ? 1 : 0);
        }
        dataBytes.append(value);
    }
    bool toggle = false;
    while (dataBytes.size() < dataCodewords) {
        dataBytes.append(toggle ? 0x11 : 0xEC);
        toggle = !toggle;
    }

    QVector<QVector<int>> blocks;
    QVector<int> cursor = dataBytes;
    int position = 0;
    for (int block = 0; block < spec.group1Blocks; ++block) {
        blocks.append(cursor.mid(position, spec.group1Data));
        position += spec.group1Data;
    }
    for (int block = 0; block < spec.group2Blocks; ++block) {
        blocks.append(cursor.mid(position, spec.group2Data));
        position += spec.group2Data;
    }
    QVector<QVector<int>> eccBlocks;
    for (const QVector<int>& block : blocks) {
        eccBlocks.append(reedSolomonRemainder(block, spec.ecPerBlock));
    }
    QVector<int> codewords;
    int maxData = 0;
    for (const QVector<int>& block : blocks) {
        maxData = std::max(maxData, static_cast<int>(block.size()));
    }
    for (int index = 0; index < maxData; ++index) {
        for (const QVector<int>& block : blocks) {
            if (index < block.size()) {
                codewords.append(block.at(index));
            }
        }
    }
    for (int index = 0; index < spec.ecPerBlock; ++index) {
        for (const QVector<int>& block : eccBlocks) {
            codewords.append(block.at(index));
        }
    }

    QVector<bool> stream;
    for (const int codeword : codewords) {
        for (int bit = 7; bit >= 0; --bit) {
            stream.append(((codeword >> bit) & 1) != 0);
        }
    }

    const int size = 17 + version * 4;
    QVector<QVector<int>> matrix(size, QVector<int>(size, 0));
    QVector<QVector<bool>> reserved(size, QVector<bool>(size, false));

    placeFinder(matrix, reserved, 0, 0);
    placeFinder(matrix, reserved, 0, size - 7);
    placeFinder(matrix, reserved, size - 7, 0);

    for (int index = 8; index < size - 8; ++index) {
        if (!reserved[6][index]) {
            matrix[6][index] = index % 2 == 0 ? 1 : 0;
            reserved[6][index] = true;
        }
        if (!reserved[index][6]) {
            matrix[index][6] = index % 2 == 0 ? 1 : 0;
            reserved[index][6] = true;
        }
    }

    const int alignments = alignmentCount(version);
    for (int row = 0; row < alignments; ++row) {
        for (int column = 0; column < alignments; ++column) {
            if (reserved[kAlignment.at(version - 1).at(row)]
                    [kAlignment.at(version - 1).at(column)]) {
                continue;
            }
            placeAlignment(matrix, reserved, kAlignment.at(version - 1).at(row),
                kAlignment.at(version - 1).at(column));
        }
    }

    matrix[size - 8][8] = 1; // dark module
    reserved[size - 8][8] = true;

    for (int index = 0; index <= 5; ++index) {
        reserved[8][index] = true;
        reserved[index][8] = true;
    }
    reserved[8][7] = true;
    reserved[8][8] = true;
    reserved[7][8] = true;
    for (int index = 0; index <= 7; ++index) {
        reserved[8][size - 1 - index] = true;
    }
    for (int index = 8; index <= 14; ++index) {
        reserved[size - 15 + index][8] = true;
    }

    if (version >= 7) {
        const int bits = versionBits(version);
        for (int index = 0; index < 18; ++index) {
            const int value = (bits >> index) & 1;
            matrix[index / 3][size - 11 + index % 3] = value;
            reserved[index / 3][size - 11 + index % 3] = true;
            matrix[size - 11 + index % 3][index / 3] = value;
            reserved[size - 11 + index % 3][index / 3] = true;
        }
    }

    int bitIndex = 0;
    bool upward = true;
    for (int right = size - 1; right >= 1; right -= 2) {
        if (right == 6) {
            right = 5;
        }
        for (int offset = 0; offset < size; ++offset) {
            const int row = upward ? size - 1 - offset : offset;
            for (int column = right; column >= right - 1; --column) {
                if (reserved[row][column]) {
                    continue;
                }
                matrix[row][column] = bitIndex < stream.size()
                        && stream.at(bitIndex)
                    ? 1
                    : 0;
                ++bitIndex;
            }
        }
        upward = !upward;
    }

    int bestMask = 0;
    int bestScore = -1;
    QVector<QVector<int>> bestMatrix;
    for (int mask = 0; mask < 8; ++mask) {
        QVector<QVector<int>> candidate = matrix;
        for (int row = 0; row < size; ++row) {
            for (int column = 0; column < size; ++column) {
                if (!reserved[row][column] && maskBit(mask, row, column)) {
                    candidate[row][column] ^= 1;
                }
            }
        }
        placeFormat(candidate, mask);
        const int score = penaltyScore(candidate);
        if (bestScore < 0 || score < bestScore) {
            bestScore = score;
            bestMask = mask;
            bestMatrix = candidate;
        }
    }
    Q_UNUSED(bestMask);

    QVector<QVector<bool>> result(size, QVector<bool>(size, false));
    for (int row = 0; row < size; ++row) {
        for (int column = 0; column < size; ++column) {
            result[row][column] = bestMatrix.at(row).at(column) != 0;
        }
    }
    return result;
}

} // namespace

QVector<QVector<bool>> qrMatrix(const QString& text)
{
    if (text.isEmpty()) {
        return {};
    }
    return qrMatrixInternal(text.toUtf8());
}

} // namespace mixvera::ui