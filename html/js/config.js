var config = {
    apiUrl: "/api/",
    mainnetExplorerUrl: "https://swap.coinscope.cc/",
    testnetExplorerUrl: "https://swaptest.coinscope.cc/",
    stagenetExplorerUrl: "http://swapstage.coinscope.cc/",
    nettype: 0, /* 0 - MAINNET, 1 - TESTNET, 2 - STAGENET */
    coinUnitPlaces: 12,
    txMinConfirms: 10,         // corresponds to CRYPTONOTE_DEFAULT_TX_SPENDABLE_AGE in Monero
    txCoinbaseMinConfirms: 60, // corresponds to CRYPTONOTE_MINED_MONEY_UNLOCK_WINDOW in Monero
    coinSymbol: 'XWP',
    openAliasPrefix: "xwp",
    coinName: 'Swap',
    coinUriPrefix: 'swap:',
    addressPrefix: 10343,
    integratedAddressPrefix: 13671,
    subAddressPrefix: 11368,
    addressPrefixTestnet: 0x5b1d,
    integratedAddressPrefixTestnet: 0x519e,
    subAddressPrefixTestnet: 0x641c,
    addressPrefixStagenet: 10343,
    integratedAddressPrefixStagenet: 13671,
    subAddressPrefixStagenet: 11368,
    feePerKB: new JSBigInt('2000000000'),//20^10 - not used anymore, as fee is dynamic.
    dustThreshold: new JSBigInt('1000000000'),//10^10 used for choosing outputs/change - we decompose all the way down if the receiver wants now regardless of threshold
    txChargeRatio: 0.5,
    defaultMixin: 10, // minimum mixin for hardfork v8 is 10 (ring size 11)
    txChargeAddress: '',
    idleTimeout: 30,
    idleWarningDuration: 20,
    maxBlockNumber: 500000000,
    avgBlockTime: 15,
    debugMode: false
};
