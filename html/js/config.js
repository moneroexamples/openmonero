var config = {
    apiUrl: "http://wallet.mineme.stream:1984/",
    mainnetExplorerUrl: "https://explorer.x-cash.org",
    testnetExplorerUrl: "https://explorer.x-cash.org",
    stagenetExplorerUrl: "https://explorer.x-cash.org",
    nettype: 0, /* 0 - MAINNET, 1 - TESTNET, 2 - STAGENET */
    coinUnitPlaces: 6,
    txMinConfirms: 10,         // corresponds to CRYPTONOTE_DEFAULT_TX_SPENDABLE_AGE in Monero
    txCoinbaseMinConfirms: 60, // corresponds to CRYPTONOTE_MINED_MONEY_UNLOCK_WINDOW in Monero
    coinSymbol: 'XCA',
    openAliasPrefix: "xca",
    coinName: 'XCASH',
    coinUriPrefix: 'XCASH:',
    addressPrefix: 0x5c134,
    integratedAddressPrefix: 19,
    subAddressPrefix: 42,
    addressPrefixTestnet: 0x16871e,
    integratedAddressPrefixTestnet: 54,
    subAddressPrefixTestnet: 63,
    addressPrefixStagenet: 24,
    integratedAddressPrefixStagenet: 25,
    subAddressPrefixStagenet: 36,
    feePerKB: new JSBigInt('2000000000'),//20^10 - not used anymore, as fee is dynamic.
    dustThreshold: new JSBigInt('1000000000'),//10^10 used for choosing outputs/change - we decompose all the way down if the receiver wants now regardless of threshold
    txChargeRatio: 0.5,
    defaultMixin: 6, // minimum mixin for hardfork v7 is 6 (ring size 7)
    txChargeAddress: '',
    idleTimeout: 30,
    idleWarningDuration: 20,
    maxBlockNumber: 500000000,
    avgBlockTime: 120,
    debugMode: false
};
