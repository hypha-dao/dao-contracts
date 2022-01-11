import { DaoBlockchain } from "./dao/DaoBlockchain";
import { setupEnvironment } from "./setup";
import { Asset } from "./types/Asset";
import { getAccountPermission } from "./utils/Permissions";
import {Dao} from "./dao/Dao";

const nextVoiceDecay = (dao: Dao, prev: Date, environment: DaoBlockchain) => {
    const next = new Date(prev);
    next.setSeconds( next.getSeconds() + dao.settings.tokens.voice.decayPeriod);
    return next;
}

describe('Voice', () => {
    it('issues and transfers', async () => {
        const environment = await setupEnvironment();
        const dao = environment.getDao('test');

        let issuedVoice = environment.getIssuedHvoice(dao);
        expect(issuedVoice).toEqual(Asset.fromString('105.00 HVOICE'));

        await expect(environment.peerContracts.voice.contract.issue({
            tenant: dao.name,
            to: dao.members[0].account.accountName,
            quantity: '50.00 HVOICE',
            memo: 'hvoice transfer'
        }, dao.members[1].getPermissions())).rejects.toThrow(/tokens can only be issued to issuer account/i);

        await expect(environment.peerContracts.voice.contract.issue({
            tenant: dao.name,
            to: environment.daoContract.accountName,
            quantity: '50.00 HVOICE',
            memo: 'hvoice transfer'
        }, dao.members[1].getPermissions())).rejects.toThrow(/missing authority of dao/i);

        await environment.peerContracts.voice.contract.issue({
            tenant: dao.name,
            to: environment.daoContract.accountName,
            quantity: '100.00 HVOICE',
            memo: 'hvoice transfer'
        }, getAccountPermission(environment.daoContract));

        issuedVoice = environment.getIssuedHvoice(dao);
        expect(issuedVoice).toEqual(Asset.fromString('205.00 HVOICE'));

        await expect(environment.peerContracts.voice.contract.transfer({
            tenant: dao.name,
            from: dao.members[1].account.accountName,
            to: dao.members[0].account.accountName,
            quantity: '50.00 HVOICE',
            memo: 'hvoice transfer'
        }, dao.members[1].getPermissions())).rejects.toThrow(/tokens can only be transferred by issuer account/i);

        await expect(environment.peerContracts.voice.contract.transfer({
            tenant: dao.name,
            from: environment.daoContract.accountName,
            to: dao.members[0].account.accountName,
            quantity: '50.00 HVOICE',
            memo: 'hvoice transfer'
        }, dao.members[1].getPermissions())).rejects.toThrow(/missing authority of dao/i);

        await environment.peerContracts.voice.contract.transfer({
            tenant: dao.name,
            from: environment.daoContract.accountName,
            to: dao.members[0].account.accountName,
            quantity: '100.00 HVOICE',
            memo: 'hvoice transfer'
        }, getAccountPermission(environment.daoContract));

        issuedVoice = environment.getIssuedHvoice(dao);
        expect(issuedVoice).toEqual(Asset.fromString('205.00 HVOICE'));

        const lastDecay = nextVoiceDecay(dao, environment.getSetupDate(), environment);
        environment.setCurrentTime(lastDecay);

        // Currently decays happen when there is a transaction
        issuedVoice = environment.getIssuedHvoice(dao);
        expect(issuedVoice).toEqual(Asset.fromString('205.00 HVOICE'));

        await environment.peerContracts.voice.contract.issue({
            tenant: dao.name,
            to: environment.daoContract.accountName,
            quantity: '50.00 HVOICE',
            memo: 'hvoice transfer'
        }, getAccountPermission(environment.daoContract));

        await environment.peerContracts.voice.contract.transfer({
            tenant: dao.name,
            from: environment.daoContract.accountName,
            to: dao.members[0].account.accountName,
            quantity: '50.00 HVOICE',
            memo: 'hvoice transfer'
        }, getAccountPermission(environment.daoContract));

        // Decayed 50% of members[0], because that's the only affected in the tx
        // members[0] had 200 so we end up with 204 - 100 + 50
        issuedVoice = environment.getIssuedHvoice(dao);
        expect(issuedVoice).toEqual(Asset.fromString('155.00 HVOICE'));

        expect(environment.getHvoiceForMember(dao, dao.members[0].account.accountName)).toEqual(
            Asset.fromString('150.00 HVOICE')
        );

    });
});
