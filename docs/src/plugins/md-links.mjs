// MkDocs-style `page.md#anchor` links -> absolute Starlight URLs (`<base>/<locale>/<slug>/#anchor`), which also lets starlight-links-validator check their anchors.
export default (base) => ({ fileURL }) => {
    const locale = /[\\/]docs[\\/](ru|zh)[\\/]/.exec(fileURL.pathname)?.[1];
    const dir = `${base}/${locale ? locale + '/' : ''}`;
    return {
        name: 'md-links',
        link(node, ctx) {
            const match = /^(?:\.\/)?([\w-]+)\.md(#.*)?$/.exec(node.url);
            if (match) ctx.setProperty(node, 'url', dir + (match[1] === 'index' ? '' : match[1] + '/') + (match[2] ?? ''));
        },
    };
};
