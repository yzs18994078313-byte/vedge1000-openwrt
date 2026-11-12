'use strict';
'require view';
'require rpc';
'require ui';
'require dom';

const callGetDiskInfo = rpc.declare({
	object: 'luci.diskman',
	method: 'get_disk_info',
	expect: { disks: [] }
});

const callEjectDisk = rpc.declare({
	object: 'luci.diskman',
	method: 'eject_disk',
	params: ['device'],
	expect: { '': {} }
});

const callSetPtable = rpc.declare({
	object: 'luci.diskman',
	method: 'set_ptable',
	params: ['device', 'label'],
	expect: { '': {} }
});

const partColors = [
	'#81ecec',
	'#74b9ff',
	'#a29bfe',
	'#55efc4',
	'#fab1a0',
	'#ffeaa7',
	'#fd79a8'
];

const formatSize = bytes => {
	if (bytes === 0) return '0 B';
	const k = 1024;
	const sizes = ['B', 'KB', 'MB', 'GB', 'TB', 'PB'];
	const i = Math.floor(Math.log(bytes) / Math.log(k));
	return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i];
};

return view.extend({
	load() {
		return callGetDiskInfo();
	},

	handleRescan() {
		ui.showModal(_('Scanning disks...'), [
			E('p', { 'class': 'spinning' }, _('Scanning disks, please wait...'))
		]);
		setTimeout(() => {
			location.reload();
		}, 500);
	},

	handleSetPtable(diskPath, label) {
		const labelName = (label === 'msdos') ? 'MBR' : 'GPT';
		if (!confirm(_('Warning: Creating a %s partition table on %s will erase all data.\nAre you sure you want to continue?').format(labelName, diskPath)))
			return;

		ui.showModal(_('Processing...'), [
			E('p', { 'class': 'spinning' }, _('Writing partition table, do not power off...'))
		]);

		callSetPtable(diskPath, label).then(res => {
			ui.hideModal();
			if (res && res.code === 0) {
				ui.addNotification(null, E('p', _('Partition table created successfully.')), 'success');
				setTimeout(() => {
					location.reload();
				}, 1000);
			} else {
				ui.addNotification(null, E('p', _('Failed to set partition table: ') + (res.error || '')), 'danger');
			}
		}).catch(e => {
			ui.hideModal();
			ui.addNotification(null, E('p', e.message), 'danger');
		});
	},

	handleEject(diskPath) {
		if (!confirm(_('Are you sure you want to eject disk %s?\nAll mounted partitions on this disk will be unmounted!').format(diskPath)))
			return;

		ui.showModal(_('Processing...'), [
			E('p', { 'class': 'spinning' }, _('Ejecting disk...'))
		]);

		callEjectDisk(diskPath).then(res => {
			ui.hideModal();
			if (res && res.code === 0) {
				ui.addNotification(null, E('p', _('Disk ejected successfully. You can now safely remove the device.')), 'success');
				setTimeout(() => {
					location.reload();
				}, 2000);
			} else {
				ui.addNotification(null, E('p', _('Eject failed: ') + (res.error || '')), 'danger');
			}
		}).catch(e => {
			ui.hideModal();
			ui.addNotification(null, E('p', e.message), 'danger');
		});
	},

	render(data) {
		const disks = data || [];
		const css = `
			.dkm-card {
				margin-bottom: 20px;
				box-shadow: 0 1px 3px rgba(0, 0, 0, 0.05);
			}
			.theme-dark .dkm-card {
				box-shadow: none;
				border: 1px solid #444;
				background: #2a2a2a;
			}
			.dkm-card-inner {
				padding: 20px;
			}
			.dkm-card-header {
				display: flex;
				justify-content: space-between;
				align-items: center;
				border-bottom: 1px solid #eee;
				padding-bottom: 15px;
				margin-bottom: 20px;
			}
			.theme-dark .dkm-card-header {
				border-bottom-color: #444;
			}
			.dkm-card-title {
				font-size: 18px;
				font-weight: bold;
				display: flex;
				align-items: center;
				gap: 8px;
			}
			.dkm-card-actions {
				display: flex;
				align-items: center;
				gap: 10px;
			}
			.dkm-card-actions select {
				padding: 4px 8px;
				font-size: 12px;
				height: auto;
			}
			.dkm-card-body {
				display: grid;
				grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
				gap: 20px;
				margin-bottom: 20px;
			}
			.dkm-item-label {
				font-size: 13px;
				color: #666;
				text-transform: uppercase;
				margin-bottom: 4px;
			}
			.theme-dark .dkm-item-label {
				color: #aaa;
			}
			.dkm-item-value {
				font-size: 16px;
				font-weight: 500;
			}
			.dm-part-bar {
				display: flex;
				width: 100%;
				height: 30px;
				background: #e4e7ed;
				border-radius: 6px;
				overflow: hidden;
				margin-top: 10px;
			}
			.theme-dark .dm-part-bar {
				background: #444;
			}
			.dm-part-segment {
				display: flex;
				align-items: center;
				justify-content: center;
				font-size: 12px;
				font-weight: bold;
				color: rgba(0, 0, 0, 0.8);
				white-space: nowrap;
				overflow: hidden;
				text-overflow: ellipsis;
				padding: 0 8px;
				border-right: 1px solid rgba(255, 255, 255, 0.4);
			}
			.dm-part-segment:last-child {
				border-right: none;
			}
			.dm-part-segment:hover {
				opacity: 0.8;
			}
			.dm-part-free {
				color: #666;
			}
			.theme-dark .dm-part-free {
				color: #aaa;
			}
			@media max-width: 600px {
				.dkm-card-header {
					flex-direction: column;
					align-items: flex-start;
					gap: 10px;
				}
				.dkm-card-actions {
					width: 100%;
					justify-content: flex-end;
				}
			}
		`;
		document.head.appendChild(E('style', { 'type': 'text/css' }, css));

		const container = E('div', { 'class': 'cbi-map' }, [
			E('h2', {}, _('DiskMan Management')),
			E('div', { 'class': 'cbi-map-descr' }, _('Manage disks via LuCI')),
			E('button', {
				'class': 'btn cbi-button cbi-button-apply',
				'style': 'margin-bottom: 20px;',
				'click': ui.createHandlerFn(this, this.handleRescan)
			}, _('Rescan Disks'))
		]);

		const disksContainer = E('div', { 'id': 'dkm-disks' });

		if (disks.length === 0) {
			disksContainer.appendChild(E('div', { 'class': 'cbi-section dkm-card' }, [
				E('div', { 'class': 'cbi-section-node dkm-card-inner' }, E('em', _('No disks found.')))
			]));
		} else {
			for (let i = 0; i < disks.length; i++) {
				const d = disks[i];

				const btnEject = E('button', {
					'class': 'btn cbi-button cbi-button-remove',
					'click': ui.createHandlerFn(this, this.handleEject, d.path)
				}, _('Eject'));

				const btnEdit = E('button', {
					'class': 'btn cbi-button cbi-button-action important',
					'click': ui.createHandlerFn(this, (devName) => {
						window.location.href = L.url('admin/system/diskman/partition', devName);
					}, d.name)
				}, _('Edit'));

				const noPtable = (!d.ptable || d.ptable === '-' || d.ptable === 'unknown');
				let ptableDisplayNode = '';
				let ptableDisplayValue = d.ptable;

				if (noPtable) {
					ptableDisplayNode = E('select', {
						'class': 'cbi-input-select',
						'change': ui.createHandlerFn(this, function (path, ev) {
							const val = ev.target.value;
							if (val) {
								this.handleSetPtable(path, val);
							}
							ev.target.value = '';
						}, d.path)
					}, [
						E('option', { 'value': '' }, _('-- Create Partition Table --')),
						E('option', { 'value': 'msdos' }, 'MBR'),
						E('option', { 'value': 'gpt' }, 'GPT')
					]);
					ptableDisplayValue = E('em', { 'style': 'color: #e6a23c;' }, _('Not Set'));

					btnEdit.disabled = true;
					btnEdit.title = _('Please create a partition table first');
					btnEdit.classList.add('disabled');
				}

				let healthText = d.health;
				if (healthText !== '-') {
					healthText = E('span', {
						'style': /PASS|OK/i.test(d.health) ? 'color: #67c23a; font-weight: bold;' : 'color: #f56c6c; font-weight: bold;'
					}, d.health);
				}

				const partBar = E('div', { 'class': 'dm-part-bar' });
				const blocksToRender = [];
				let currentOffset = 0;
				const GAP_THRESHOLD = 5242880; // 5MB

				d.partitions.sort((a, b) => a.start - b.start);

				for (let j = 0; j < d.partitions.length; j++) {
					const p = d.partitions[j];
					const gapSize = p.start - currentOffset;

					if (gapSize > GAP_THRESHOLD) {
						blocksToRender.push({
							type: 'free',
							size: gapSize,
							label: _('Unallocated') + ' ' + formatSize(gapSize)
						});
					}

					blocksToRender.push({
						type: 'part',
						size: p.size,
						label: p.name + ' ' + p.fs_type + ' ' + formatSize(p.size),
						colorIndex: j
					});

					currentOffset = Math.max(currentOffset, p.start + p.size);
				}

				const endGap = d.size - currentOffset;
				if (endGap > GAP_THRESHOLD) {
					blocksToRender.push({
						type: 'free',
						size: endGap,
						label: _('Unallocated') + ' ' + formatSize(endGap)
					});
				}

				let minWidthPct = 8;
				if (blocksToRender.length > 0 && blocksToRender.length * minWidthPct > 100) {
					minWidthPct = Math.floor(100 / blocksToRender.length);
				}

				for (let k = 0; k < blocksToRender.length; k++) {
					const block = blocksToRender[k];
					const percent = d.size > 0 ? ((block.size / d.size) * 100) : 0;

					if (block.type === 'free') {
						partBar.appendChild(E('div', {
							'class': 'dm-part-segment dm-part-free',
							'style': 'flex-grow: ' + percent + '; flex-shrink: 1; min-width: ' + minWidthPct + '%;',
							'title': block.label
						}, block.label));
					} else {
						const bgColor = partColors[block.colorIndex % partColors.length];
						partBar.appendChild(E('div', {
							'class': 'dm-part-segment',
							'style': 'flex-grow: ' + percent + '; flex-shrink: 1; min-width: ' + minWidthPct + '%; background-color: ' + bgColor + ';',
							'title': block.label
						}, block.label));
					}
				}

				const cardActions = [ptableDisplayNode, btnEject, btnEdit];

				const diskCard = E('div', { 'class': 'cbi-section dkm-card' }, [
					E('div', { 'class': 'cbi-section-node dkm-card-inner' }, [
						E('div', { 'class': 'dkm-card-header' }, [
							E('div', { 'class': 'dkm-card-title' }, [
								E('span', {}, d.model + ' (' + d.path + ')')
							]),
							E('div', { 'class': 'dkm-card-actions' }, cardActions)
						]),
						E('div', { 'class': 'dkm-card-body' }, [
							E('div', { 'class': 'dkm-item' }, [E('div', { 'class': 'dkm-item-label' }, _('Size')), E('div', { 'class': 'dkm-item-value' }, formatSize(d.size))]),
							E('div', { 'class': 'dkm-item' }, [E('div', { 'class': 'dkm-item-label' }, _('Serial Number')), E('div', { 'class': 'dkm-item-value' }, d.serial)]),
							E('div', { 'class': 'dkm-item' }, [E('div', { 'class': 'dkm-item-label' }, _('Partition Table')), E('div', { 'class': 'dkm-item-value' }, ptableDisplayValue)]),
							E('div', { 'class': 'dkm-item' }, [E('div', { 'class': 'dkm-item-label' }, _('Health Status')), E('div', { 'class': 'dkm-item-value' }, healthText)]),
							E('div', { 'class': 'dkm-item' }, [E('div', { 'class': 'dkm-item-label' }, _('Temperature')), E('div', { 'class': 'dkm-item-value' }, d.temp)]),
							E('div', { 'class': 'dkm-item' }, [E('div', { 'class': 'dkm-item-label' }, _('NVMe/SATA Version')), E('div', { 'class': 'dkm-item-value' }, d.sata_ver)])
						]),
						E('div', { 'class': 'dkm-part-container' }, [
							E('div', { 'style': 'font-size: 0.9rem; font-weight: bold; margin-bottom: 8px;' }, _('Partitions')),
							partBar
						])
					])
				]);

				disksContainer.appendChild(diskCard);
			}
		}

		container.appendChild(E('h3', _('Disks')));
		container.appendChild(disksContainer);

		return container;
	},

	handleSaveApply: null,
	handleSave: null,
	handleReset: null
});
