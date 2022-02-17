import Account from '@klevoya/hydra/lib/main/account';

export const getAccountPermission = (account: Account | string): import('eosjs/dist/eosjs-serialize').Authorization[] => {
    return [
        {
            actor: typeof account === 'string' ? account : account.accountName,
            permission: 'active'
        }
    ];
}
