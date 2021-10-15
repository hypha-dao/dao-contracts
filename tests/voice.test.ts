import { DaoBlockchain } from "./dao/DaoBlockchain";
import { setupEnvironment } from "./setup";
import { Asset } from "./types/Asset";
import { getAccountPermission } from "./utils/Permissions";

const nextVoiceDecay = (prev: Date, environment: DaoBlockchain) => {
    const next = new Date(prev);
    next.setSeconds( next.getSeconds() + environment.settings.voice.decayPeriod);
    return next;
}

describe('Voice', () => {
    it('issues and transfers', async () => {
        const environment = await setupEnvironment();
        let issuedVoice = environment.getIssuedHvoice();
        expect(issuedVoice).toEqual(Asset.fromString('104.00 HVOICE'));

        await expect(environment.peerContracts.voice.contract.issue({
            to: environment.members[0].account.accountName,
            quantity: '50.00 HVOICE',
            memo: 'hvoice transfer'
        }, environment.members[1].getPermissions())).rejects.toThrow(/tokens can only be issued to issuer account/i);

        await expect(environment.peerContracts.voice.contract.issue({
            to: environment.dao.accountName,
            quantity: '50.00 HVOICE',
            memo: 'hvoice transfer'
        }, environment.members[1].getPermissions())).rejects.toThrow(/missing authority of dao/i);

        await environment.peerContracts.voice.contract.issue({
            to: environment.dao.accountName,
            quantity: '100.00 HVOICE',
            memo: 'hvoice transfer'
        }, getAccountPermission(environment.dao));

        issuedVoice = environment.getIssuedHvoice();
        expect(issuedVoice).toEqual(Asset.fromString('204.00 HVOICE'));
        
        await expect(environment.peerContracts.voice.contract.transfer({
            from: environment.members[1].account.accountName,
            to: environment.members[0].account.accountName,
            quantity: '50.00 HVOICE',
            memo: 'hvoice transfer'
        }, environment.members[1].getPermissions())).rejects.toThrow(/tokens can only be transferred by issuer account/i);

        await expect(environment.peerContracts.voice.contract.transfer({
            from: environment.dao.accountName,
            to: environment.members[0].account.accountName,
            quantity: '50.00 HVOICE',
            memo: 'hvoice transfer'
        }, environment.members[1].getPermissions())).rejects.toThrow(/missing authority of dao/i);

        await environment.peerContracts.voice.contract.transfer({
            from: environment.dao.accountName,
            to: environment.members[0].account.accountName,
            quantity: '100.00 HVOICE',
            memo: 'hvoice transfer'
        }, getAccountPermission(environment.dao));

        issuedVoice = environment.getIssuedHvoice();
        expect(issuedVoice).toEqual(Asset.fromString('204.00 HVOICE'));

        const lastDecay = nextVoiceDecay(environment.getSetupDate(), environment);
        environment.setCurrentTime(lastDecay);

        // Currently decays happen when there is a transaction
        issuedVoice = environment.getIssuedHvoice();
        expect(issuedVoice).toEqual(Asset.fromString('204.00 HVOICE'));
        
        await environment.peerContracts.voice.contract.issue({
            to: environment.dao.accountName,
            quantity: '50.00 HVOICE',
            memo: 'hvoice transfer'
        }, getAccountPermission(environment.dao));

        await environment.peerContracts.voice.contract.transfer({
            from: environment.dao.accountName,
            to: environment.members[0].account.accountName,
            quantity: '50.00 HVOICE',
            memo: 'hvoice transfer'
        }, getAccountPermission(environment.dao));

        // Decayed 50% of members[0], because that's the only affected in the tx
        // members[0] had 200 so we end up with 204 - 100 + 50
        issuedVoice = environment.getIssuedHvoice();
        expect(issuedVoice).toEqual(Asset.fromString('154.00 HVOICE'));

        expect(environment.getHvoiceForMember(environment.members[0].account.accountName)).toEqual(
            Asset.fromString('150.00 HVOICE')
        );

    });
});
