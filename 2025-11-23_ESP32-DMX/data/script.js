// DMX channels configuration
const channelDefs = [
    // Ch1 (Yaw) and Ch2 (Pitch) are handled by the XY Pad
    {
        ch: 3,
        label: "Color Wheel",
        type: "range_select",
        ranges: [
            { label: "Gobo/Def", min: 0, max: 71 },
            { label: "Static", min: 72, max: 139 },
            { label: "Auto", min: 140, max: 255 }
        ]
    },
    {
        ch: 4,
        label: "Gobo Wheel",
        type: "range_select",
        ranges: [
            { label: "Open", min: 0, max: 7 },
            { label: "Gobo 1", min: 8, max: 15 },
            { label: "Gobo 2", min: 16, max: 23 },
            { label: "Gobo 3", min: 24, max: 31 },
            { label: "Gobo 4", min: 32, max: 39 },
            { label: "Gobo 5", min: 40, max: 47 },
            { label: "Gobo 6", min: 48, max: 55 },
            { label: "Gobo 7", min: 56, max: 63 },
            { label: "W. Shake", min: 64, max: 71 },
            { label: "Shake 1", min: 72, max: 79 },
            { label: "Shake 2", min: 80, max: 87 },
            { label: "Shake 3", min: 88, max: 95 },
            { label: "Shake 4", min: 96, max: 103 },
            { label: "Shake 5", min: 104, max: 111 },
            { label: "Shake 6", min: 112, max: 119 },
            { label: "Shake 7", min: 120, max: 127 },
            { label: "Auto", min: 128, max: 255 }
        ]
    },
    { ch: 5, label: "Strobe", type: "slider" },
    { ch: 6, label: "Master Dimmer", type: "slider" },
    { ch: 7, label: "Movement Speed", type: "slider" },
    {
        ch: 8,
        label: "Auto Mode",
        type: "range_select",
        ranges: [
            { label: "Inactive", min: 0, max: 134 },
            { label: "Auto", min: 135, max: 159 },
            { label: "Sound 3", min: 160, max: 184 },
            { label: "Sound 2", min: 185, max: 209 },
            { label: "Sound 1", min: 210, max: 234 },
            { label: "Sound 0", min: 235, max: 255 }
        ]
    },
    {
        ch: 9,
        label: "Reset",
        type: "button",
        buttonLabel: "RESET DEVICE",
        value: 255
    },
];

const channelsContainer = document.getElementById("channels");
const statusEl = document.getElementById("status");
const allOffBtn = document.getElementById("allOffBtn");
const allFullBtn = document.getElementById("allFullBtn");

// XY Pad Elements
const xyPad = document.getElementById("xyPad");
const xyHandle = document.getElementById("xyHandle");
const valX = document.getElementById("valX");
const valY = document.getElementById("valY");

let isDragging = false;

function setStatus(text, ok = true) {
    statusEl.textContent = text;
    // statusEl.classList.toggle("ok", ok);
    // statusEl.classList.toggle("err", !ok);
}

function sendDMXValue(ch, value) {
    fetch(`/set?ch=${encodeURIComponent(ch)}&val=${encodeURIComponent(value)}`)
        .then((res) => {
            if (!res.ok) throw new Error(res.statusText);
            setStatus(`Set ch ${ch} → ${value}`, true);
        })
        .catch((err) => {
            console.error(err);
            setStatus(`Error sending ch ${ch}`, false);
        });
}

// --- XY Pad Logic ---
function updateXY(x, y) {
    // Clamp values 0-255
    x = Math.max(0, Math.min(255, Math.round(x)));
    y = Math.max(0, Math.min(255, Math.round(y)));

    // Update UI
    valX.textContent = x;
    valY.textContent = y;

    // Position handle (0-100%)
    const pctX = (x / 255) * 100;
    const pctY = (y / 255) * 100;
    xyHandle.style.left = `${pctX}%`;
    xyHandle.style.top = `${pctY}%`;

    // Send DMX
    sendDMXValue(1, x);
    sendDMXValue(2, y);
}

function handleDrag(e) {
    if (!isDragging) return;

    // Support touch and mouse events
    const clientX = e.touches ? e.touches[0].clientX : e.clientX;
    const clientY = e.touches ? e.touches[0].clientY : e.clientY;

    const rect = xyPad.getBoundingClientRect();
    const offsetX = clientX - rect.left;
    const offsetY = clientY - rect.top;

    // Calculate 0-255 based on position within the pad
    let x = (offsetX / rect.width) * 255;
    let y = (offsetY / rect.height) * 255;

    updateXY(x, y);
}

xyPad.addEventListener("mousedown", (e) => {
    isDragging = true;
    xyHandle.classList.add("active");
    handleDrag(e);
});

xyPad.addEventListener("touchstart", (e) => {
    isDragging = true;
    xyHandle.classList.add("active");
    handleDrag(e);
    e.preventDefault(); // Prevent scrolling
}, { passive: false });

window.addEventListener("mousemove", handleDrag);
window.addEventListener("touchmove", handleDrag, { passive: false });

window.addEventListener("mouseup", () => {
    if (isDragging) {
        isDragging = false;
        xyHandle.classList.remove("active");
    }
});

window.addEventListener("touchend", () => {
    if (isDragging) {
        isDragging = false;
        xyHandle.classList.remove("active");
    }
});

// --- Channel Row Logic ---
function createChannelRow(def) {
    const row = document.createElement("div");
    row.className = "channel-row";

    // Header: Label + Value
    const header = document.createElement("div");
    header.className = "ch-header";

    const label = document.createElement("div");
    label.className = "ch-label";
    label.textContent = `Ch ${def.ch} — ${def.label}`;

    const valueBox = document.createElement("div");
    valueBox.className = "value-box";
    valueBox.textContent = "0";

    header.appendChild(label);
    header.appendChild(valueBox);
    row.appendChild(header);

    let controlContainer = document.createElement("div");

    if (def.type === "range_select") {
        // 1. Create Range Buttons
        const capsContainer = document.createElement("div");
        capsContainer.className = "range-capsules";

        // 2. Create Slider
        const slider = document.createElement("input");
        slider.type = "range";
        slider.min = "0";
        slider.max = "255";
        slider.value = "0";

        // State to track current range
        let currentMin = 0;
        let currentMax = 255;

        // Helper to update slider constraints
        const setRange = (min, max, btn) => {
            currentMin = min;
            currentMax = max;

            // Update visual active state
            Array.from(capsContainer.children).forEach(b => b.classList.remove("active"));
            if (btn) btn.classList.add("active");

            // Clamp slider value to new range
            let val = parseInt(slider.value);
            if (val < min) val = min;
            if (val > max) val = max;

            slider.min = min;
            slider.max = max;
            slider.value = val;

            valueBox.textContent = val;
            sendDMXValue(def.ch, val);
        };

        def.ranges.forEach((r, idx) => {
            const btn = document.createElement("button");
            btn.className = "range-btn";
            btn.textContent = r.label;

            // Default active if 0 is in range
            if (idx === 0) btn.classList.add("active");

            btn.addEventListener("click", () => {
                setRange(r.min, r.max, btn);
            });

            capsContainer.appendChild(btn);
        });

        // Slider Logic
        slider.addEventListener("input", () => {
            valueBox.textContent = slider.value;
        });

        slider.addEventListener("change", () => {
            sendDMXValue(def.ch, slider.value);
        });

        // Initialize first range
        if (def.ranges.length > 0) {
            slider.min = def.ranges[0].min;
            slider.max = def.ranges[0].max;
        }

        controlContainer.appendChild(capsContainer);
        controlContainer.appendChild(slider);

        // Store for bulk updates
        def.control = slider;
        def.setRange = setRange; // Expose for bulk reset
        def.ranges = def.ranges; // Expose for bulk reset
        def.capsContainer = capsContainer;

    } else if (def.type === "button") {
        const btn = document.createElement("button");
        btn.className = "action-btn";
        btn.textContent = def.buttonLabel || "Trigger";

        btn.addEventListener("click", () => {
            const v = def.value || 255;
            valueBox.textContent = v;
            sendDMXValue(def.ch, v);
            setTimeout(() => valueBox.textContent = "-", 1000);
        });
        valueBox.textContent = "-";
        controlContainer.appendChild(btn);

    } else {
        // Default Slider
        const slider = document.createElement("input");
        slider.type = "range";
        slider.min = "0";
        slider.max = "255";
        slider.value = "0";

        slider.addEventListener("input", () => {
            valueBox.textContent = slider.value;
        });

        slider.addEventListener("change", () => {
            sendDMXValue(def.ch, slider.value);
        });

        controlContainer.appendChild(slider);
        def.control = slider;
    }

    row.appendChild(controlContainer);
    def.valueBox = valueBox;

    return row;
}

// Build UI
channelDefs.forEach((def) => {
    const row = createChannelRow(def);
    channelsContainer.appendChild(row);
});

// Bulk controls
allOffBtn.addEventListener("click", () => {
    // Reset XY Pad
    updateXY(0, 0);

    // Reset other channels
    channelDefs.forEach((def) => {
        if (def.type === "slider") {
            def.control.value = 0;
            def.valueBox.textContent = "0";
            sendDMXValue(def.ch, 0);
        } else if (def.type === "range_select") {
            // Reset to first range
            const firstRange = def.ranges[0];
            const firstBtn = def.capsContainer.children[0];
            def.setRange(firstRange.min, firstRange.max, firstBtn);

            def.control.value = firstRange.min;
            def.valueBox.textContent = firstRange.min;
            sendDMXValue(def.ch, firstRange.min);
        }
    });
});

allFullBtn.addEventListener("click", () => {
    // Set XY Pad to full? Or maybe center? Let's do full for consistency
    updateXY(255, 255);

    channelDefs.forEach((def) => {
        if (def.type === "slider") {
            def.control.value = 255;
            def.valueBox.textContent = "255";
            sendDMXValue(def.ch, 255);
        }
        // Skip range_select for safety/complexity
    });
});

// Optional: sync UI from /state endpoint
function fetchState() {
    fetch("/state")
        .then((res) => res.json())
        .then((json) => {
            // Sync XY Pad
            const x = Number(json[1]) || 0;
            const y = Number(json[2]) || 0;

            // Update UI without sending DMX (avoid loop)
            valX.textContent = x;
            valY.textContent = y;
            const pctX = (x / 255) * 100;
            const pctY = (y / 255) * 100;
            xyHandle.style.left = `${pctX}%`;
            xyHandle.style.top = `${pctY}%`;

            // Sync other channels
            channelDefs.forEach((def) => {
                const v = Number(json[def.ch]) || 0;

                if (def.type === "slider") {
                    def.control.value = v;
                } else if (def.type === "range_select") {
                    // Find which range this value belongs to
                    const rangeIdx = def.ranges.findIndex(r => v >= r.min && v <= r.max);
                    if (rangeIdx !== -1) {
                        const range = def.ranges[rangeIdx];
                        const btn = def.capsContainer.children[rangeIdx];
                        def.setRange(range.min, range.max, btn);
                        def.control.value = v;
                    }
                }

                if (def.valueBox) def.valueBox.textContent = v;
            });
            setStatus("Synced from ESP32", true);
        })
        .catch(() => {
            // It's okay if /state isn't implemented
        });
}

setTimeout(fetchState, 1000);
