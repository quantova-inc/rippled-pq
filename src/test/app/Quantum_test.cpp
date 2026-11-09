//------------------------------------------------------------------------------
/*
    This file is part of rippled: https://github.com/ripple/rippled
    Copyright (c) 2024 Ripple Labs Inc.

    Permission to use, copy, modify, and/or distribute this software for any
    purpose  with  or without fee is hereby granted, provided that the above
    copyright notice and this permission notice appear in all copies.

    THE  SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
    WITH  REGARD  TO  THIS  SOFTWARE  INCLUDING  ALL  IMPLIED  WARRANTIES  OF
    MERCHANTABILITY  AND  FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
    ANY  SPECIAL ,  DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
    WHATSOEVER  RESULTING  FROM  LOSS  OF USE, DATA OR PROFITS, WHETHER IN AN
    ACTION  OF  CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/
//==============================================================================

#include <test/jtx.h>
#include <test/jtx/batch.h>

#include <xrpl/protocol/Feature.h>
#include <xrpl/protocol/TxFlags.h>
#include <xrpl/protocol/jss.h>

namespace ripple {
namespace test {

class Quantum_test : public beast::unit_test::suite
{
    static void
    setForceQuantum(jtx::Env& env, jtx::Account const& account)
    {
        using namespace jtx;
        env(fset(account, asfForceQuantum));
        env.close();
    }

    void
    testSingleSignRejectsClassical(FeatureBitset features)
    {
        testcase("single sign rejects classical key on force quantum account");

        using namespace jtx;
        Env env{*this, envconfig(), features};
        Account const alice{"alice"};
        Account const bob{"bob"};
        env.fund(XRP(1000), alice, bob);
        env.close();

        setForceQuantum(env, alice);

        env(pay(alice, bob, XRP(1)), ter(telBAD_PUBLIC_KEY));
        env.close();
    }

    void
    testSingleSignRejectsRegularKey(FeatureBitset features)
    {
        testcase("single sign rejects classical regular key");

        using namespace jtx;
        Env env{*this, envconfig(), features};
        Account const alice{"alice"};
        Account const bob{"bob"};
        Account const regular{"regular"};
        env.fund(XRP(1000), alice, bob);
        env.close();

        env(regkey(alice, regular));
        env.close();

        setForceQuantum(env, alice);

        env(pay(alice, bob, XRP(1)), sig(regular), ter(telBAD_PUBLIC_KEY));
        env.close();
    }

    void
    testMultiSignRejectsClassical(FeatureBitset features)
    {
        testcase(
            "multi sign rejects classical signer on force quantum account");

        using namespace jtx;
        Env env{*this, envconfig(), features};
        Account const alice{"alice"};
        Account const bob{"bob"};
        Account const carol{"carol"};
        env.fund(XRP(1000), alice, bob, carol);
        env.close();

        env(signers(alice, 1, {{bob, 1}, {carol, 1}}));
        env.close();

        setForceQuantum(env, alice);

        auto const baseFee = env.current()->fees().base;
        env(pay(alice, carol, XRP(1)),
            msig(bob),
            fee(3 * baseFee),
            ter(telBAD_PUBLIC_KEY));
        env.close();
    }

    void
    testBatchInnerSignerRejectsClassical(FeatureBitset features)
    {
        testcase("batch inner signer rejects classical key");

        using namespace jtx;
        Env env{*this, envconfig(), features};
        Account const alice{"alice"};
        Account const bob{"bob"};
        Account const carol{"carol"};
        env.fund(XRP(1000), alice, bob, carol);
        env.close();

        setForceQuantum(env, bob);

        auto const seq = env.seq(alice);
        auto const batchFee = batch::calcBatchFee(env, 1, 2);
        env(batch::outer(alice, seq, batchFee, tfAllOrNothing),
            batch::inner(pay(alice, carol, XRP(1)), seq + 1),
            batch::inner(pay(bob, carol, XRP(1)), env.seq(bob)),
            batch::sig(bob),
            ter(telBAD_PUBLIC_KEY));
        env.close();
    }

    void
    testClassicalStillAllowedWithoutForceQuantum(FeatureBitset features)
    {
        testcase("classical signing still allowed without force quantum");

        using namespace jtx;
        Env env{*this, envconfig(), features};
        Account const alice{"alice"};
        Account const bob{"bob"};
        env.fund(XRP(1000), alice, bob);
        env.close();

        env(pay(alice, bob, XRP(1)), ter(tesSUCCESS));
        env.close();
    }

public:
    void
    run() override
    {
        using namespace jtx;
        auto const features = supported_amendments();
        testSingleSignRejectsClassical(features);
        testSingleSignRejectsRegularKey(features);
        testMultiSignRejectsClassical(features);
        testBatchInnerSignerRejectsClassical(features);
        testClassicalStillAllowedWithoutForceQuantum(features);
    }
};

BEAST_DEFINE_TESTSUITE(Quantum, app, ripple);

}  // namespace test
}  // namespace ripple
