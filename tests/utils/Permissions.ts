import Account from '@klevoya/hydra/lib/main/account';

export const getAccountPermission = (account: Account): import('eosjs/dist/eosjs-serialize').Authorization[] => {
    return [
        {
            actor: account.accountName,
            permission: 'active'
        } 
    ];
}
