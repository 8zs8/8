function initApp() {
  let resizeTimer;
  window.addEventListener('resize', () => {
    clearTimeout(resizeTimer);
    resizeTimer = setTimeout(() => location.reload(), 200);
  });
  const studentData = `陈泓睿,男
陈建业,男
陈雨钒,男
邓可妍,女
邓晓彰,女
郭瑾萱,女
何丞杰,男
何弘彬,男
何金霖,女
何乐瑶,女
何泰燃,男
何芷莹,女
黄可蓝,女
黄紫涵,女
劳晓晴,女
劳梓谦,男
李睿诚,男
李思亭,女
李栩婷,女
梁思璐,女
梁炜晋,男
梁倚彤,女
林晨菲,女
林剑涛,男
林若曦,女
林诗悦,女
林毅泳,男
林紫瑶,女
刘辰铭,男
楼应铖,男
罗淳蓝,女
罗晓娜,女
马健朗,男
麦耀桦,男
庞嘉,女
彭璐,女
秦嘉骏,男
谭嘉烨,女
文俊锋,男
吴恩哲,男
肖童,女
谢昊晋,男
谢彤彤,女
徐绰延,男
徐铭浩,男
杨佳祺,女
袁梓豪,男
张嘉荣,男
张紫淇,女`.trim().split('\n').map(line => {
    const [name, gender] = line.split(',');
    return { name, gender: gender.trim() };
  }).filter(i => i.name);
  let currentMode = "all";
  const canvasContainer = document.getElementById('canvas-container');
  const uiOverlay = document.querySelector('.ui-overlay');
  uiOverlay.innerHTML = `
    <div class="title">课堂3D随机点名系统</div>
    <div class="center-name">
      <div id="selectedNameAnimator"></div>
    </div>
    <div class="control-panel">
      <div class="mode-select">
        <button class="mode-btn mode-all active">全班</button>
        <button class="mode-btn mode-boy">只抽男生</button>
        <button class="mode-btn mode-girl">只抽女生</button>
      </div>
      <div class="slider-wrapper">
        <span class="slider-label">🐢 慢</span>
        <input type="range" id="speedSlider" min="0" max="100" value="40">
        <span class="slider-label">⚡ 快</span>
      </div>
      <button class="action-btn" id="actionBtn">开始抽问</button>
      <div class="footer-info">当前模式：全班抽问 | 共49人（男23人/女26人）</div>
    </div>
  `;
  const footerInfo = document.querySelector('.footer-info');
  document.querySelector('.mode-all').onclick = () => {
    currentMode = 'all';
    setActive(0);
    footerInfo.textContent = "当前模式：全班抽问 | 共49人（男23人/女26人）";
  };
  document.querySelector('.mode-boy').onclick = () => {
    currentMode = 'boy';
    setActive(1);
    footerInfo.textContent = "当前模式：只抽男生 | 共23人";
  };
  document.querySelector('.mode-girl').onclick = () => {
    currentMode = 'girl';
    setActive(2);
    footerInfo.textContent = "当前模式：只抽女生 | 共26人";
  };
  function setActive(index) {
    document.querySelectorAll('.mode-btn').forEach((b, i) => {
      b.classList.toggle('active', i === index);
    });
  }
  function getList() {
    if (currentMode === 'boy') return studentData.filter(s => s.gender === '男');
    if (currentMode === 'girl') return studentData.filter(s => s.gender === '女');
    return studentData;
  }
  const scene = new THREE.Scene();
  const camera = new THREE.PerspectiveCamera(55, innerWidth/innerHeight, 0.1, 200);
  camera.position.set(0, 3, 18);
  const renderer = new THREE.WebGLRenderer({ antialias:true, alpha:true });
  renderer.setSize(innerWidth, innerHeight);
  canvasContainer.appendChild(renderer.domElement);
  const labelRenderer = new THREE.CSS2DRenderer();
  labelRenderer.setSize(innerWidth, innerHeight);
  labelRenderer.domElement.style.cssText = 'position:absolute;top:0;left:0;pointer-events:none;';
  canvasContainer.appendChild(labelRenderer.domElement);
  const group = new THREE.Group();
  scene.add(group);
  const R = 8;
  const phi = Math.PI * (3 - Math.sqrt(5));
  const tags = [];
  studentData.forEach((stu, i) => {
    const y = 1 - (i/(studentData.length-1))*2;
    const r = Math.sqrt(1-y*y);
    const t = phi*i;
    const x = Math.cos(t)*r;
    const z = Math.sin(t)*r;
    const div = document.createElement('div');
    div.className = 'name-tag';
    div.textContent = stu.name;
    const obj = new THREE.CSS2DObject(div);
    obj.position.set(x*R, y*R, z*R);
    group.add(obj);
    tags.push(obj);
  });
  let rolling = false;
  let speed = 0.03;
  const slider = document.getElementById('speedSlider');
  const btn = document.getElementById('actionBtn');
  const show = document.getElementById('selectedNameAnimator');
  slider.oninput = () => {
    speed = 0.01 + (slider.value/100)*0.12;
  };
  btn.onclick = () => {
    rolling = !rolling;
    btn.textContent = rolling ? '停止抽问' : '开始抽问';
    if (!rolling) {
      const list = getList();
      const rand = list[Math.floor(Math.random()*list.length)];
      show.textContent = rand.name;
      show.classList.add('animate-to-center');
    } else {
      show.classList.remove('animate-to-center');
    }
  };
  function animate(){
    requestAnimationFrame(animate);
    if(rolling){
      group.rotation.y += speed;
      group.rotation.x += speed*0.3;
    }
    tags.forEach(t=>t.lookAt(camera.position));
    renderer.render(scene,camera);
    labelRenderer.render(scene,camera);
  }
  animate();
}