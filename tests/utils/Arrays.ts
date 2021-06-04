export const last = <T>(arr: Array<T>): T => {
    if (arr.length === 0) {
        throw new Error('Empty array');
    }

    return arr[arr.length - 1];
}
