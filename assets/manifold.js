const latexify = function (M) {
  if (M.length == 0) {
    return "\\emptyset";
  }

  const ncols = M[0].length;
  let tex = `\\left[ \\begin{array}{${"c".repeat(ncols)}}\n`;

  for (let i = 0; i < M.length; i++) {
    tex += "\t";
    for (let j = 0; j < ncols; j++) {
      tex += `${M[i][j]}`;
      if (j < ncols - 1) {
        tex += " & ";
      }
    }
    tex += " \\\\ \n";
  }
  tex += "\\end{array} \\right]";
  return tex;
};

const latexify_set_of_sets = function (M) {
  if (M.length == 0) {
    return "\\emptyset";
  }

  const ncols = M[0].length;
  let tex = "\\Big\\{";

  const skipSome = M.length * ncols > 15;

  for (let i = 0; i < M.length; i++) {
    if (skipSome && i > 1 && i < M.length - 2) {
      if (i == 2) {
        tex += "\\dots,";
      }
      continue;
    }

    tex += "(";
    for (let j = 0; j < ncols; j++) {
      tex += M[i][j];
      if (j < ncols - 1) {
        tex += ", ";
      }
    }
    tex += ")";

    if (i < M.length - 1) {
      tex += ", ";
    }
  }
  tex += "\\Big\\}";
  return tex;
};

const latexify_time_series = function (x, numObs) {
  if (numObs == 0) {
    return "\\emptyset";
  }

  let tex = `\\begin{array}{c|c}\n`;
  tex += "\\text{Time} & \\text{Value} \\\\ \n \\hline ";
  for (let i = 1; i <= numObs; i++) {
    tex += `t_{${i}} & ${x}_{${i}} \\\\ \n`;
  }
  tex += "\\end{array}";

  return tex;
};

const manifold = function (x, numObs, E, tau, allowMissing = false, p = 1) {
  let M = [];
  if (E == 0) {
    return M;
  }

  let targets = [];

  for (let i = 1; i <= numObs; i++) {
    const targetInd = i + p;
    const targetMissing = targetInd < 1 || targetInd > numObs;
    const target = targetMissing ? "\\text{NA}" : `${x}_{${targetInd}}`;

    let M_i = [];
    let hasMissing = false;
    for (let j = 0; j < E; j++) {
      ind = i - j * tau;
      indMissing = ind < 1 || ind > numObs;
      if (indMissing) {
        M_i.push("\\text{NA}");
      } else {
        M_i.push(`${x}_{${ind}}`);
      }
      hasMissing |= indMissing;
    }

    if (hasMissing && !allowMissing) {
      continue;
    }
    M.push(M_i);
    targets.push([target]);
  }
  return { manifold: M, targets: targets };
};

const add_slider_outputs = function () {
  const sliderContainers = Array.from(
    document.querySelectorAll(".slider-container")
  );
  const sliders = document.querySelectorAll(".slider-container input");

  const sliderDisplays = sliderContainers.map((container) =>
    container.appendChild(document.createElement("span"))
  );

  // Update the slider ouputs
  sliders.forEach(function (slider, i) {
    slider.addEventListener("input", function () {
      sliderDisplays[i].innerHTML = `${this.value}`;
    });
  });

  // Trigger event right away to display default values of sliders
  sliders.forEach((slider) => slider.dispatchEvent(new Event("input")));
};

add_slider_outputs();
