#include "PackedBits.hpp"

#include <QtTest>

namespace {
enum class Mode : u8 { Off, Low, High, Max };
enum class Delta : i8 { Down = -2, Zero = 0, Up = 3 };

using Small = PackedBits<Field<"flag", bool, 1>, Field<"mode", Mode, 2>, Field<"count", u8, 5>>;
using Signed = PackedBits<Field<"neg", i8, 4>, Field<"wide", i32, 20>, Field<"delta", Delta, 4>>;
// 12 + 60 + 7 + 64 = 143 bits: a field straddles every word boundary
using Wide = PackedBits<Field<"head", u16, 12>, Field<"cross", u64, 60>, Field<"tail", u8, 7>, Field<"full", u64, 64>>;

static_assert(Small::bit_count == 8);
static_assert(Small::size_bytes == 1);
static_assert(PackedBits<Field<"a", u16, 9>>::size_bytes == 2);
static_assert(PackedBits<Field<"a", u32, 17>>::size_bytes == 4);
static_assert(PackedBits<Field<"a", u64, 33>>::size_bytes == 8);
static_assert(Wide::bit_count == 143);
static_assert(Wide::size_bytes == 24);

constexpr auto smallRoundTrips() -> bool {
    Small bits(true, Mode::High, 21);
    return bits.get<"flag">() && bits.get<"mode">() == Mode::High && bits.get<"count">() == 21;
}

static_assert(smallRoundTrips());
}  // namespace

class TestPackedBits final : public QObject {
    Q_OBJECT

   private slots:
    void defaultIsZero() {
        const Small small;
        QVERIFY(!small.get<"flag">());
        QCOMPARE(small.get<"mode">(), Mode::Off);
        QCOMPARE(small.get<"count">(), u8{ 0 });

        const Wide wide;
        QCOMPARE(wide.get<"head">(), u16{ 0 });
        QCOMPARE(wide.get<"cross">(), u64{ 0 });
        QCOMPARE(wide.get<"full">(), u64{ 0 });
    }

    void constructorSetsEveryField() {
        const Small bits(true, Mode::Low, 31);

        QVERIFY(bits.get<"flag">());
        QCOMPARE(bits.get<"mode">(), Mode::Low);
        QCOMPARE(bits.get<"count">(), u8{ 31 });
    }

    void fieldsDoNotBleed() {
        Small bits;

        bits.set<"count">(31);
        QVERIFY(!bits.get<"flag">());
        QCOMPARE(bits.get<"mode">(), Mode::Off);

        bits.set<"mode">(Mode::Max);
        QCOMPARE(bits.get<"count">(), u8{ 31 });
        QVERIFY(!bits.get<"flag">());

        bits.set<"flag">(true);
        bits.set<"count">(0);
        QVERIFY(bits.get<"flag">());
        QCOMPARE(bits.get<"mode">(), Mode::Max);
        QCOMPARE(bits.get<"count">(), u8{ 0 });
    }

    void overwritingClearsOldBits() {
        Small bits;
        bits.set<"count">(0b11111);
        bits.set<"count">(0b00100);

        QCOMPARE(bits.get<"count">(), u8{ 0b00100 });
    }

    // A value wider than its field is truncated to the field, not spilled into the next one
    void oversizedValuesAreMasked() {
        Small bits;
        bits.set<"mode">(Mode::Off);
        bits.set<"count">(0xFF);

        QCOMPARE(bits.get<"count">(), u8{ 31 });
        QVERIFY(!bits.get<"flag">());
        QCOMPARE(bits.get<"mode">(), Mode::Off);
    }

    void signedFieldsExtendTheirSign() {
        Signed bits;

        bits.set<"neg">(-3);
        bits.set<"wide">(-100000);
        bits.set<"delta">(Delta::Down);

        QCOMPARE(bits.get<"neg">(), i8{ -3 });
        QCOMPARE(bits.get<"wide">(), i32{ -100000 });
        QCOMPARE(bits.get<"delta">(), Delta::Down);

        bits.set<"neg">(7);
        bits.set<"wide">(524287);  // 2^19 - 1, the largest value of a 20-bit signed field
        bits.set<"delta">(Delta::Up);

        QCOMPARE(bits.get<"neg">(), i8{ 7 });
        QCOMPARE(bits.get<"wide">(), i32{ 524287 });
        QCOMPARE(bits.get<"delta">(), Delta::Up);
    }

    void signedFieldsKeepTheirNeighbours() {
        Signed bits(-8, -524288, Delta::Zero);

        QCOMPARE(bits.get<"neg">(), i8{ -8 });
        QCOMPARE(bits.get<"wide">(), i32{ -524288 });
        QCOMPARE(bits.get<"delta">(), Delta::Zero);

        bits.set<"neg">(1);
        QCOMPARE(bits.get<"wide">(), i32{ -524288 });
    }

    void wideStorageRoundTrips() {
        Wide bits;

        bits.set<"head">(0xABC);
        bits.set<"cross">(0x0FEDCBA987654321ULL);
        bits.set<"tail">(0x55);
        bits.set<"full">(0xDEADBEEFCAFEF00DULL);

        QCOMPARE(bits.get<"head">(), u16{ 0xABC });
        QCOMPARE(bits.get<"cross">(), u64{ 0x0FEDCBA987654321ULL });
        QCOMPARE(bits.get<"tail">(), u8{ 0x55 });
        QCOMPARE(bits.get<"full">(), u64{ 0xDEADBEEFCAFEF00DULL });
    }

    void wideFieldsAcrossWordBoundaryDoNotBleed() {
        Wide bits;

        bits.set<"full">(~u64{ 0 });
        bits.set<"cross">(0);
        bits.set<"head">(0);
        bits.set<"tail">(0);

        QCOMPARE(bits.get<"full">(), ~u64{ 0 });

        bits.set<"cross">(0x0FFFFFFFFFFFFFFFULL);
        QCOMPARE(bits.get<"head">(), u16{ 0 });
        QCOMPARE(bits.get<"tail">(), u8{ 0 });
        QCOMPARE(bits.get<"full">(), ~u64{ 0 });

        bits.set<"cross">(0);
        QCOMPARE(bits.get<"full">(), ~u64{ 0 });
        QCOMPARE(bits.get<"cross">(), u64{ 0 });
    }

    void wideConstructor() {
        const Wide bits(0x123, 0x0123456789ABCDEFULL, 0x7F, 0xFFFFFFFFFFFFFFFFULL);

        QCOMPARE(bits.get<"head">(), u16{ 0x123 });
        QCOMPARE(bits.get<"cross">(), u64{ 0x0123456789ABCDEFULL });
        QCOMPARE(bits.get<"tail">(), u8{ 0x7F });
        QCOMPARE(bits.get<"full">(), ~u64{ 0 });
    }

    void sixtyFourBitScalarField() {
        PackedBits<Field<"all", u64, 64>> bits;

        bits.set<"all">(0x8000000000000001ULL);
        QCOMPARE(bits.get<"all">(), u64{ 0x8000000000000001ULL });
    }
};

QTEST_APPLESS_MAIN(TestPackedBits)
#include "tst_packedbits.moc"
