export const toISOString = (date: Date) => {
    // The testing framework always set the mills to 000 and doesn't have the Z
    return date.toISOString().split('.')[0] + '.000';
};
